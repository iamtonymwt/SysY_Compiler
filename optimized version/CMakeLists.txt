cmake_minimum_required(VERSION 3.20)
project(Optimize)

set(CMAKE_CXX_STANDARD 14)

add_executable(Optimize main.cpp lexical.h lexical.cpp grammer.h grammer.cpp vTable.h vTable.cpp midcode.h midcode.cpp mipscode.h mipscode.cpp optimize.cpp optimize.h)
