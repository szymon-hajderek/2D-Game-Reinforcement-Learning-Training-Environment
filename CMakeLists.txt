cmake_minimum_required(VERSION 3.28)          # 2025 LTS line
project(game LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(COMPILE_FLAGS -O3)

# If you want static libs: set(SFML_STATIC_LIBRARIES ON) *before* find_package.
find_package(SFML 3.0.1 CONFIG REQUIRED COMPONENTS Graphics Window System)

# Example executable
add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE SFML::Graphics)

# Main game executable
add_executable(game src/main.cpp)
target_link_libraries(game PRIVATE SFML::Graphics)