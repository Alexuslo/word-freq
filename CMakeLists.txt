cmake_minimum_required(VERSION 3.13)
project(WordFreqTask LANGUAGES CXX)

# ТЗ не требует Linux/macOS: маппинг файла — Win32 CreateFileMapping.
if(NOT WIN32)
    message(FATAL_ERROR "WordFreqTask targets Windows (Win32 memory-mapped files).")
endif()

add_executable(word_freq
    main.cpp
    WordFreqApp.cpp
    WordFreqApp.hpp
    ChunkProcessor.cpp
    ChunkProcessor.hpp
    Constants.hpp
    Signal.hpp
    sha256.hpp
)

target_compile_features(word_freq PRIVATE cxx_std_17)

# Не подменять CMAKE_CXX_FLAGS_RELEASE целиком: иначе теряется /DNDEBUG.
if(MSVC)
    target_compile_options(word_freq PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/GL>
    )
    target_link_options(word_freq PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT word_freq)

set_target_properties(word_freq PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(word_freq_tests
        tests/word_freq_tests.cpp
        ChunkProcessor.cpp
        ChunkProcessor.hpp
        Constants.hpp
        sha256.hpp
    )
    target_include_directories(word_freq_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(word_freq_tests PRIVATE cxx_std_17)
    add_test(NAME word_freq_tests COMMAND word_freq_tests)
endif()
