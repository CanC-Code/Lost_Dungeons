cmake_minimum_required(VERSION 3.22.1)

project("lostdungeons")

# Automatically download and link the nlohmann/json library for parsing
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

add_library(engine SHARED
        engine_core.cpp
        asset_manager.cpp
        jni_bridge.cpp)

find_library(log-lib log)
find_library(android-lib android) # Required for AAssetManager

target_link_libraries(engine PRIVATE 
        ${log-lib} 
        ${android-lib} 
        nlohmann_json::nlohmann_json)
