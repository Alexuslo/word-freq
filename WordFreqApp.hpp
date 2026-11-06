#pragma once

#include "Signal.hpp"

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

// Owns the input mapping, worker pool and completion signal.
// Ready indices are delivered out of order so Reduce can start before the last Map finishes.
class WordFreqApp
{
public:
    WordFreqApp() = default;
    ~WordFreqApp();

    WordFreqApp(const WordFreqApp&) = delete;
    WordFreqApp& operator=(const WordFreqApp&) = delete;

    int Init(int argc, char* argv[]);
    int Run();

private:
    void ReleaseMapping();

    std::string m_InputPath;
    std::string m_OutputPath;

    void* m_HFile = nullptr;
    void* m_HMap = nullptr;
    const char* m_Data = nullptr;
    size_t m_FileSize = 0;

    Signal<int> m_OnChunkCompleted;

    std::mutex m_QueueMutex;
    std::condition_variable m_CV;
    std::vector<int> m_ReadyIndices;
};
