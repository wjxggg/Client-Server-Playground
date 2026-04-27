cmake_minimum_required(VERSION 4.0)

project(game)

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Cmake options
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/lib) # Static lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/bin) # Shared lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/bin) # Executable
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/lib) # Static lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/bin) # Shared lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/bin) # Executable

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wall")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Public files
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/src/third-party)

# Engine targets
add_subdirectory(${CMAKE_SOURCE_DIR}/src/engine/Client)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/engine/Server)
