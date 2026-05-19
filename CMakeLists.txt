cmake_minimum_required(VERSION 3.10)
project(WordFreqTask)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Максимальная оптимизация для Release сборки
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")
endif()

add_executable(word_freq 
    main.cpp 
    WordFreqApp.cpp 
    WordFreqApp.hpp
    ChunkProcessor.cpp 
    ChunkProcessor.hpp
    Constants.hpp
    GlobalSignals.hpp
    Signal.hpp
    sha256.hpp
)

# Устанавливаем наш проект в качестве стартового (по умолчанию) в Visual Studio, 
# чтобы избежать запуска ALL_BUILD при нажатии F5
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT word_freq)

# При запуске из Visual Studio (по F5) устанавливаем рабочую директорию на папку с исходниками,
# чтобы программа видела дефолтный файл input.txt
set_target_properties(word_freq PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
