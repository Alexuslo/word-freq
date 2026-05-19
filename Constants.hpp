#pragma once

#include <cstddef>
#include <string_view>

namespace Config
{
    constexpr std::string_view DefaultInputFile = "input.txt";
    constexpr std::string_view DefaultOutputFile = "result.txt";
    // Фолбэк числа потоков, если hardware_concurrency() вернул 0
    constexpr unsigned int DefaultThreadCount = 4;
    // Максимальная длина слова (символов)
    constexpr size_t MaxWordLength = 255;
    // Буфер под слово: MaxWordLength + 1 (запас под '\0')
    constexpr size_t WordBufferSize = MaxWordLength + 1;
}
