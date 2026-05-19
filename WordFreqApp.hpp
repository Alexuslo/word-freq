#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

// Главный класс приложения. Реализует паттерн Singleton для удобного глобального доступа, обычная практика в GameDev.
// Отвечает за инициализацию ресурсов (файлов), запуск потоков и Event Loop (оркестрацию).
class WordFreqApp
{
public:
    static WordFreqApp& Get()
    {
        static WordFreqApp m_Instance;
        return m_Instance;
    }

    // Инициализация путей, открытие файлов и маппинг в память
    int Init(int argc, char* argv[]);
    // Запуск пула потоков, Event Loop для слияния данных, сортировка и запись результата
    int Run();
    // Корректное освобождение системных ресурсов
    void Release();

private:
    WordFreqApp() = default;
    ~WordFreqApp() = default;

    WordFreqApp(const WordFreqApp&) = delete;
    WordFreqApp& operator=(const WordFreqApp&) = delete;

    std::string m_InputPath;
    std::string m_OutputPath;

    // Ресурсы Windows API для Memory-Mapped File
    void* m_HFile = nullptr;
    void* m_HMap = nullptr;
    const char* m_Data = nullptr; // Указатель на начало файла в памяти
    size_t m_FileSize = 0;

    // Примитивы для асинхронной очереди (Event Loop)
    // Мьютекс защищает только доступ к очереди индексов, но не сам процесс слияния.
    std::mutex m_QueueMutex;
    // Condition Variable позволяет главному потоку "спать", пока нет готовых данных,
    // не потребляя процессорное время (избавляет от busy-wait).
    std::condition_variable m_CV;
    // "Почтовый ящик" - очередь, куда рабочие потоки складывают свои индексы по завершении.
    std::vector<int> m_ReadyIndices;
};
