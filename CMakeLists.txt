cmake_minimum_required(VERSION 4.4)
project(gpu_particles)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} src/main.cpp
        src/App.cpp
        src/App.h)

set(CMAKE_BUILD_TYPE Debug)

target_compile_options(${PROJECT_NAME} PRIVATE
        -g3
        -O0
)

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)

find_package(SDL3_shadercross CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_shadercross::SDL3_shadercross)

find_package(SDL3_ttf CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf)