cmake_minimum_required(VERSION 3.0.0)
project(XCIOp LANGUAGES C VERSION 0.1.0)

add_executable(XCIOp main.c)
if (MSVC)
    target_compile_options(XCIOp PRIVATE /D_CRT_SECURE_NO_WARNINGS)
else ()
    target_compile_options(XCIOp PRIVATE -D_FILE_OFFSET_BITS=64)
endif ()
