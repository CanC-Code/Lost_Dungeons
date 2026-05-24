cmake_minimum_required(VERSION 3.22.1)

project("lostdungeons")

include(FetchContent)

# JSON Parser
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# NEW: GLM (OpenGL Mathematics)
FetchContent_Declare(glm URL https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip)
FetchContent_MakeAvailable(glm)

add_library(engine SHARED
        engine_core.cpp
        asset_manager.cpp
        gl_renderer.cpp
        jni_bridge.cpp)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

target_link_libraries(engine PRIVATE 
        ${log-lib} 
        ${android-lib} 
        ${egl-lib}
        ${gles-lib}
        nlohmann_json::nlohmann_json
        glm::glm) # Link GLM
