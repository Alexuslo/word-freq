#pragma once

#include <cstddef>
#include <string_view>

namespace Config
{
    constexpr std::string_view DefaultInputFile = "input.txt";
    constexpr std::string_view DefaultOutputFile = "result.txt";
    // Фаллбэк количества потоков если hardware_concurrency() вернет 0
    constexpr unsigned int DefaultThreadCount = 4;
    // Макс размер буфера
    constexpr size_t MaxWordLength = 255;
    // Размер буфера под слово: MaxWordLength + 1 (запас под '\0', хотя длину храним отдельно)
    constexpr size_t WordBufferSize = MaxWordLength + 1;
}
