# Minimum version of CMake required
cmake_minimum_required(VERSION 3.1)

# Project name
project(SorryMCTS)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
add_compile_options(-O3 -Wall)

# Source files
set(SRC_FILES
  action.cpp
  card.cpp
  common.cpp
  deck.cpp
  main.cpp
  playerColor.cpp
  sorry.cpp
  sorryMcts.cpp
)

# Header files
set(INC_FILES
  action.hpp
  card.hpp
  common.hpp
  deck.hpp
  playerColor.hpp
  sorry.hpp
  sorryMcts.hpp
)

# Build executable
add_executable(${PROJECT_NAME} ${SRC_FILES} ${INC_FILES})
