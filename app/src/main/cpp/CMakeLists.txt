cmake_minimum_required(VERSION 3.22.1)

project("lostdungeons")

add_library(engine SHARED
        engine_core.cpp
        jni_bridge.cpp)

find_library(log-lib log)

target_link_libraries(engine ${log-lib})
