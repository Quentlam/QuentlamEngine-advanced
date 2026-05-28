cmake_minimum_required(VERSION 3.10)
project(QuentlamEditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SRC_FILES "QuentlamEngine/src/*.cpp" "QuentlamEngine/src/*.h" "QL-Editor/src/*.cpp" "QL-Editor/src/*.h")

add_executable(QuentlamEditor ${SRC_FILES})

# Includes
target_include_directories(QuentlamEditor PRIVATE
    "QuentlamEngine/src"
    "QuentlamEngine/vendor"
    "QuentlamEngine/vendor/spdlog/include"
    "QuentlamEngine/vendor/glm"
    "QuentlamEngine/vendor/imgui"
    "QuentlamEngine/vendor/entt/src"
    "QuentlamEngine/vendor/Glad/include"
    "QuentlamEngine/vendor/Box2D/include"
    "QuentlamEngine/vendor/JoltPhysics"
    "QuentlamEngine/vendor/ImGuizmo"
    "QuentlamEngine/vendor/miniaudio"
    "QL-Editor/src"
)

# Definitions
target_compile_definitions(QuentlamEditor PRIVATE 
    QL_PLATFORM_WINDOWS
    ENTT_INCLUDE_NATVIS
)

if(WIN32)
    target_link_libraries(QuentlamEditor PRIVATE opengl32.lib)
else()
    target_link_libraries(QuentlamEditor PRIVATE GL)
endif()
