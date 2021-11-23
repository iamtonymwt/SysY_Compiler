cmake_minimum_required(VERSION 3.20)
project(Mips)

set(CMAKE_CXX_STANDARD 14)

add_executable(Mips main.cpp lexical.h lexical.cpp grammer.h grammer.cpp midcode.cpp midcode.h vTable.cpp vTable.h mipscode.cpp mipscode.h)
