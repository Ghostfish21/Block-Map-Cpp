cmake_minimum_required(VERSION 4.0)
project(BlockMapCPP)

set(CMAKE_CXX_STANDARD 11)

add_executable(BlockMapCPP main.cpp
        Chunk.cpp
        Chunk.h
        BlockIdFacingSet.cpp
        BlockIdFacingSet.h
        FlatFacing.h
        BlockSet.cpp
        BlockSet.h
        StorageSlot.cpp
        StorageSlot.h
        StorageSlot.cpp
        ArrayStack.cpp
        ArrayStack.h
        benchmark.cpp
        benchmark.h
)
