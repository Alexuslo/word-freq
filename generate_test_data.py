import argparse
import random
import os
import time

def generate_file(output_path, size_mb):
    latin_words = [
        'hello', 'world', 'test', 'chunking', 'thread', 'future', 'mutex', 'async', 
        'performance', 'code', 'data', 'file', 'memory', 'mapped', 'word', 'frequency',
        'system', 'algorithm', 'function', 'variable', 'object', 'class', 'method',
        'pointer', 'reference', 'compiler', 'linker', 'build', 'release', 'debug',
        'architecture', 'network', 'database', 'server', 'client', 'request', 'response',
        'socket', 'stream', 'buffer', 'cache', 'packet', 'queue', 'stack', 'tree',
        'graph', 'node', 'edge', 'vertex', 'hash', 'map', 'dictionary', 'array',
        'list', 'vector', 'string', 'integer', 'float', 'boolean', 'character', 'byte',
        'bit', 'module', 'package', 'library', 'framework', 'interface', 'protocol',
        'abstract', 'virtual', 'static', 'dynamic', 'inline', 'const', 'volatile',
        'atomic', 'lock', 'condition', 'event', 'signal', 'slot', 'observer', 'pattern',
        'singleton', 'factory', 'builder', 'adapter', 'proxy', 'facade', 'decorator',
        'state', 'strategy', 'command', 'visitor', 'iterator', 'loop', 'branch',
        'condition', 'switch', 'case', 'break', 'continue', 'return', 'yield', 'throw',
        'catch', 'finally', 'try', 'except', 'error', 'exception', 'warning', 'log',
        'trace', 'metric', 'monitor', 'profile', 'optimize', 'benchmark', 'scale'
    ]

    cyrillic_words = [
        'привет', 'мир', 'тест', 'поток', 'данные', 'файл', 'память', 'слово', 
        'частота', 'задача', 'многопоточность', 'скорость', 'оптимизация', 'результат', 
        'код', 'программа', 'строка', 'функция', 'указатель', 'массив',
        'процессор', 'ядро', 'архитектура', 'система', 'очередь', 'сообщение', 'сигнал',
        'буфер', 'кеш', 'сеть', 'сервер', 'клиент', 'запрос', 'ответ', 'пакет',
        'сокет', 'поток', 'граф', 'дерево', 'узел', 'ребро', 'вершина', 'хеш',
        'карта', 'словарь', 'вектор', 'список', 'число', 'символ', 'байт', 'бит',
        'модуль', 'пакет', 'библиотека', 'фреймворк', 'интерфейс', 'протокол',
        'абстракция', 'виртуальный', 'статический', 'динамический', 'встраиваемый',
        'константа', 'атомарный', 'блокировка', 'условие', 'событие', 'слот',
        'наблюдатель', 'паттерн', 'одиночка', 'фабрика', 'строитель', 'адаптер',
        'прокси', 'фасад', 'декоратор', 'состояние', 'стратегия', 'команда', 'посетитель',
        'итератор', 'цикл', 'ветвление', 'переключатель', 'возврат', 'выброс', 'перехват',
        'ошибка', 'исключение', 'предупреждение', 'журнал', 'трассировка', 'метрика',
        'мониторинг', 'профилирование', 'оптимизация', 'масштабирование', 'проект',
        'репозиторий', 'коммит', 'слияние', 'ветка', 'индекс', 'узел', 'состояние'
    ]

    all_words = latin_words + cyrillic_words
    target_size_bytes = size_mb * 1024 * 1024

    print(f"[*] Генерация файла {output_path} размером ~{size_mb} МБ...")
    start_time = time.time()

    bytes_written = 0
    
    # Используем кодировку cp1251 (Windows-1251), так как C++ код оптимизирован под нее
    with open(output_path, 'w', encoding='cp1251') as f:
        while bytes_written < target_size_bytes:
            # Генерируем текст большими батчами для высокой скорости I/O
            batch = ' '.join(random.choices(all_words, k=5000)) + '\n'
            f.write(batch)
            bytes_written += len(batch.encode('cp1251'))
            
            # Простой прогресс-бар
            progress = (bytes_written / target_size_bytes) * 100
            if random.random() < 0.05: # Обновляем не на каждой итерации для скорости
                print(f"\r[~] Прогресс: {progress:.1f}%", end='', flush=True)

    end_time = time.time()
    real_size_mb = os.path.getsize(output_path) / (1024 * 1024)
    
    print(f"\r[+] Готово! 100.0%                    ")
    print(f"[*] Фактический размер: {real_size_mb:.2f} МБ")
    print(f"[*] Время генерации: {end_time - start_time:.2f} секунд")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Генератор больших текстовых файлов для Word Frequency Task")
    parser.add_argument('-o', '--output', type=str, default='input.txt', help='Путь к выходному файлу (по умолчанию: input.txt)')
    parser.add_argument('-s', '--size', type=int, default=100, help='Целевой размер файла в мегабайтах (по умолчанию: 100)')
    
    args = parser.parse_args()
    generate_file(args.output, args.size)
