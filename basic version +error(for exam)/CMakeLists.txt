cmake_minimum_required(VERSION 3.20)
project(Mips)

set(CMAKE_CXX_STANDARD 14)

add_executable(Mips
        main.cpp lexical.h lexical.cpp
        error_grammer.cpp error_grammer.h error.cpp error.h
        mips_grammer.cpp mips_grammer.h
        midcode.cpp midcode.h vTable.cpp vTable.h mipscode.cpp mipscode.h)
