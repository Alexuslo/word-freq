#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

// ChunkProcessor отвечает за чистое вычисление (Worker).
// Этот класс не имеет состояния (stateless), поэтому его методы статические.
// Он ничего не знает о файлах, мьютексах или потоках. Он получает часть памяти,
// обрабатывает ее по определенным правилам и возвращает результат.
class ChunkProcessor 
{
public:
    // Главный метод обработки чанка файла.
    // Возвращает локальный частотный словарь для обработанного диапазона.
    static std::unordered_map<std::string, uint64_t> Process(const char* const Data, size_t Size, size_t Start, size_t End);

private:
    // Вспомогательные методы для работы с текстом
    static bool IsAlpha(char c);
    static char ToLower(char c);
};
