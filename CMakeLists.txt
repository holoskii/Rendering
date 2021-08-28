# set versions
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)

# set name
project(RayTracing)

# set directories
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# include headers and source files
include_directories("${PROJECT_SOURCE_DIR}/include")
FILE(GLOB CPP_SOURCES "src/*.cpp")

# add code files
add_executable(RayTracing ${H_HEADERS} ${CPP_SOURCES})

# include thread support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(RayTracing PRIVATE Threads::Threads)