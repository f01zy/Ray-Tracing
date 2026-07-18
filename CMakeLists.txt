cmake_minimum_required(VERSION 3.5)
project(ray_tracing C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(TARGET ray-tracing)
set(SOURCE ./src/main.c)

add_executable(${TARGET} ${SOURCE})
add_subdirectory(./third_party/cglm)

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${TARGET} PRIVATE cglm m)
