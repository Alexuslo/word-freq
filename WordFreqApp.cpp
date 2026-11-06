#include "WordFreqApp.hpp"
#include "ChunkProcessor.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
unsigned char Cp1251SortKey(unsigned char Ch)
{
    // ё (0xB8) должен идти после е (0xE5) и перед ж (0xE6).
    return Ch == 0xB8 ? 0xE5 : Ch;
}

bool Cp1251Less(const std::string& A, const std::string& B)
{
    const size_t Common = std::min(A.size(), B.size());
    for (size_t i = 0; i < Common; ++i)
    {
        const unsigned char La = Cp1251SortKey(static_cast<unsigned char>(A[i]));
        const unsigned char Lb = Cp1251SortKey(static_cast<unsigned char>(B[i]));
        if (La != Lb)
        {
            return La < Lb;
        }
        if (A[i] != B[i])
        {
            return static_cast<unsigned char>(A[i]) < static_cast<unsigned char>(B[i]);
        }
    }
    return A.size() < B.size();
}
}

WordFreqApp::~WordFreqApp()
{
    ReleaseMapping();
}

void WordFreqApp::ReleaseMapping()
{
    if (m_Data)
    {
        UnmapViewOfFile(m_Data);
        m_Data = nullptr;
    }

    if (m_HMap)
    {
        CloseHandle(m_HMap);
        m_HMap = nullptr;
    }

    if (m_HFile)
    {
        CloseHandle(m_HFile);
        m_HFile = nullptr;
    }
}

int WordFreqApp::Init(int argc, char* argv[])
{
    if (argc >= 3)
    {
        m_InputPath = argv[1];
        m_OutputPath = argv[2];
    }
    else if (argc == 2)
    {
        m_InputPath = argv[1];
        m_OutputPath = Config::DefaultOutputFile;
        std::cout << "Output file not specified. Using default: " << m_OutputPath << "\n";
    }
    else
    {
        m_InputPath = Config::DefaultInputFile;
        m_OutputPath = Config::DefaultOutputFile;
        std::cout << "Arguments not specified. Using defaults: " << m_InputPath << " -> " << m_OutputPath << "\n";
    }

    HANDLE HFile = CreateFileA(m_InputPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (HFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open input file: " << m_InputPath << "\n";
        return 1;
    }
    m_HFile = HFile;

    LARGE_INTEGER li;
    if (!GetFileSizeEx(HFile, &li))
    {
        std::cerr << "Failed to get file size.\n";
        return 1;
    }
    m_FileSize = static_cast<size_t>(li.QuadPart);
    if (m_FileSize == 0)
    {
        return 0;
    }

    HANDLE HMap = CreateFileMappingA(HFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!HMap)
    {
        std::cerr << "Failed to create file mapping.\n";
        return 1;
    }
    m_HMap = HMap;

    m_Data = static_cast<const char*>(MapViewOfFile(HMap, FILE_MAP_READ, 0, 0, 0));
    if (!m_Data)
    {
        std::cerr << "Failed to map view of file.\n";
        return 1;
    }

    return 0;
}

int WordFreqApp::Run()
{
    if (m_FileSize == 0)
    {
        std::ofstream OutFile(m_OutputPath);
        if (!OutFile.is_open())
        {
            std::cerr << "Failed to open output file for writing.\n";
            return 1;
        }
        return 0;
    }

    if (!m_Data)
    {
        std::cerr << "File is not loaded.\n";
        return 1;
    }

    unsigned int NumThreads = std::thread::hardware_concurrency();
    if (NumThreads == 0)
    {
        NumThreads = Config::DefaultThreadCount;
    }

    std::cout << "Using " << NumThreads << " threads for processing." << std::endl;

    m_OnChunkCompleted.DisconnectAll();
    m_ReadyIndices.clear();
    m_ReadyIndices.reserve(NumThreads);

    m_OnChunkCompleted.Connect([this](int index) {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_ReadyIndices.push_back(index);
        m_CV.notify_one();
    });

    const char* const DataPtr = m_Data;
    const size_t FileSize = m_FileSize;
    const size_t ChunkSize = FileSize / NumThreads;

    // Per-worker slots. Each worker writes only its own index; main reads an index
    // only after that worker's Emit, which happens after the write.
    std::vector<std::unordered_map<std::string, uint64_t>> ChunkResults(NumThreads);
    std::vector<std::exception_ptr> ChunkErrors(NumThreads);
    std::vector<std::future<void>> Futures;

    for (unsigned int i = 0; i < NumThreads; ++i)
    {
        const size_t start = static_cast<size_t>(i) * ChunkSize;
        const size_t end = (i == NumThreads - 1) ? FileSize : start + ChunkSize;

        Futures.push_back(std::async(std::launch::async,
            [this, DataPtr, FileSize, start, end, i, &ChunkResults, &ChunkErrors]() {
                try
                {
                    ChunkResults[i] = ChunkProcessor::Process(DataPtr, FileSize, start, end);
                }
                catch (...)
                {
                    ChunkErrors[i] = std::current_exception();
                }
                try
                {
                    m_OnChunkCompleted.Emit(static_cast<int>(i));
                }
                catch (...)
                {
                    std::lock_guard<std::mutex> Lock(m_QueueMutex);
                    m_ReadyIndices.push_back(static_cast<int>(i));
                    m_CV.notify_one();
                }
            }));
    }

    std::unordered_map<std::string, uint64_t> CommonMap;
    unsigned int ProcessedCount = 0;

    while (ProcessedCount < NumThreads)
    {
        std::vector<int> ReadyNow;
        {
            std::unique_lock<std::mutex> Lock(m_QueueMutex);
            m_CV.wait(Lock, [this] { return !m_ReadyIndices.empty(); });
            ReadyNow = std::move(m_ReadyIndices);
            m_ReadyIndices.clear();
        }

        for (int i : ReadyNow)
        {
            if (ChunkErrors[static_cast<size_t>(i)])
            {
                m_OnChunkCompleted.DisconnectAll();
                std::rethrow_exception(ChunkErrors[static_cast<size_t>(i)]);
            }

            for (const auto& [word, count] : ChunkResults[static_cast<size_t>(i)])
            {
                CommonMap[word] += count;
            }
            ProcessedCount++;
        }
    }

    m_OnChunkCompleted.DisconnectAll();

    std::vector<std::pair<std::string, uint64_t>> SortedWords(CommonMap.begin(), CommonMap.end());
    std::sort(SortedWords.begin(), SortedWords.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second)
        {
            return a.second > b.second;
        }
        return Cp1251Less(a.first, b.first);
    });

    std::ofstream OutFile(m_OutputPath);
    if (!OutFile.is_open())
    {
        std::cerr << "Failed to open output file for writing.\n";
        return 1;
    }

    for (const auto& p : SortedWords)
    {
        OutFile << p.first << ": " << p.second << "\n";
    }

    std::cout << "Processing completed successfully.\n";
    return 0;
}
