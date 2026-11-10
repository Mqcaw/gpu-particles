cmake_minimum_required(VERSION 4.4)
project(gpu_particles)

set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME} src/main.cpp
        src/App.cpp
        src/App.h
        src/Shaders.cpp
        src/Shaders.h)

# compile shaders
find_program(SHADERCROSS_EXE shadercross
        HINTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sdl3-shadercross"
        REQUIRED)

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

file(GLOB SHADER_SOURCES  CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/*.hlsl")
file(GLOB SHADER_INCLUDES CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/*.hlsli")

set(SHADER_OUTPUTS)

foreach(SHADER_SRC ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SRC} NAME_WLE)   # particles.comp.hlsl -> particles.comp
    set(SHADER_SPV "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
            OUTPUT  ${SHADER_SPV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
            COMMAND ${SHADERCROSS_EXE} ${SHADER_SRC}
            -s HLSL
            -d SPIRV
            -I ${SHADER_SOURCE_DIR}
            -o ${SHADER_SPV}
            DEPENDS ${SHADER_SRC} ${SHADER_INCLUDES}
            COMMENT "Compiling shader ${SHADER_NAME}.hlsl -> SPIR-V"
            VERBATIM)

    list(APPEND SHADER_OUTPUTS ${SHADER_SPV})
endforeach()

add_custom_target(shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} shaders)

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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders)