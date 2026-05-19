#include "WordFreqApp.hpp"
#include "ChunkProcessor.hpp"
#include "Constants.hpp"
#include "GlobalSignals.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <future>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <windows.h>

int WordFreqApp::Init(int argc, char* argv[])
{
    // Парсинг аргументов командной строки.
    // Если файлы не переданы, используются дефолтные значения из Constants.hpp.
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

    // memory mapped file
    // Для максимальной производительности чтения файла используется механизм memory mapped files.
    // Вместо чтения файла кусками (через read() или ifstream) файл проецируется напрямую в виртуальную память.
    // Это избавляет от двойного копирования (ядро ОС -> буфер приложения) и позволяет обращаться к 
    // данным как к обычному массиву (m_data). Сама ОС берет на себя ленивую подгрузку страниц (Page Faults).
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
    m_FileSize = li.QuadPart;

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
    if (!m_Data || m_FileSize == 0)
    {
        std::cerr << "File is empty or not loaded.\n";
        return 0;
    }

    // Определяем количество логических потоков (включая HT)
    unsigned int NumThreads = std::thread::hardware_concurrency();
    if (NumThreads == 0) NumThreads = Config::DefaultThreadCount;
    
    std::cout << "Using " << NumThreads << " threads for processing." << std::endl;

    m_ReadyIndices.clear();

    // Главный поток (Мейн) передает воркерам "инструкцию" (лямбду),
    // которая будет вызвана в конце работы рабочего потока.
    // Лямбда выполняет только быструю потокобезопасную передачу индекса потока в "почтовый ящик".
    AppEvents::OnChunkCompleted.Connect([this](int index) {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_ReadyIndices.push_back(index);
        m_CV.notify_one(); // Будим мейн
    });

    std::vector<std::future<std::unordered_map<std::string, uint64_t>>> Futures;
    size_t ChunkSize = m_FileSize / NumThreads;

    const char* const DataPtr = m_Data;
    size_t FileSize = m_FileSize;

    // Запускаем пулл рабочих потоков
    for (unsigned int i = 0; i < NumThreads; ++i) {
        size_t start = i * ChunkSize;
        size_t end = (i == NumThreads - 1) ? FileSize : start + ChunkSize;

        Futures.push_back(std::async(std::launch::async, [DataPtr, FileSize, start, end, i]() {
            // Воркер делает свою тяжелую математическую работу (парсинг и хэширование)
            auto LocalMap = ChunkProcessor::Process(DataPtr, FileSize, start, end);
            
            // Воркер обращается к глобальной шине событий
            AppEvents::OnChunkCompleted.Emit(i);
            
            return LocalMap;
        }));
    }

    std::unordered_map<std::string, uint64_t> CommonMap;
    unsigned int ProcessedCount = 0;

    // Мы не блокируемся жестко на futures[0].get() (Head-of-Line Blocking).
    // Вместо этого мы спим и просыпаемся, когда ЛЮБОЙ рабочий поток готов отдать данные.
    while (ProcessedCount < NumThreads)
    {
        std::vector<int> ReadyNow;
        {
            std::unique_lock<std::mutex> Lock(m_QueueMutex);
            // Если очередь пуста, полностью засыпаем (освобождаем процессор) и отпускаем мьютекс.
            // Просыпаемся только по notify_one() из лямбды-воркера.
            m_CV.wait(Lock, [this] { return !m_ReadyIndices.empty(); });
            
            // Чтобы не держать мьютекс во время долгого мержа и не блокировать другие воркеры,
            // мы мгновенно копируем (перемещаем) все индексы из "почтового ящика" себе в локальный блокнот.
            ReadyNow = std::move(m_ReadyIndices);
            m_ReadyIndices.clear();
        }

        // Фаза Reduce (Слияние данных)
        // Выполняется БЕЗ мьютексов, параллельно с фазой Map (парсингом других чанков).
        for (int i : ReadyNow)
        {
            auto LocalMap = Futures[i].get(); // Получаем данные (это мгновенно, т.к. поток уже вызвал Emit)
            for (const auto& [word, count] : LocalMap)
            {
                CommonMap[word] += count; // Суммируем частоты одинаковых слов
            }
            ProcessedCount++;
        }
    }

    // Перекладываем данные из хеш-таблицы (где порядок не гарантирован) в вектор.
    std::vector<std::pair<std::string, uint64_t>> SortedWords(CommonMap.begin(), CommonMap.end());
    std::sort(SortedWords.begin(), SortedWords.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second)
        {
            return a.second > b.second; // По убыванию частоты
        }

        return a.first < b.first; // При равной частоте сортируем по алфавиту (для стабильного вывода)
    });

    // Запись результата в файл
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

void WordFreqApp::Release()
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
