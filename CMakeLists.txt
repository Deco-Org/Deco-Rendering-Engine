cmake_minimum_required(VERSION 3.20)
project(deco_rendering_engine VERSION 1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(rendering_engine STATIC
    src/rendering_engine.cpp
    include/rendering_engine_api.cpp
    src/metal_implementation.cpp
    src/stb_image_impl.cpp
    src/utils/AAPLMathUtilities.cpp
    src/texture/texture.cpp
    src/ufbx_implementation.cpp
    src/mesh/mesh.cpp
    src/asset_systems/texture_loader.cpp
    src/asset_systems/material_system.cpp
    src/asset_systems/submesh_system.cpp
    src/core_systems/transformation_system.cpp
    src/metal/render_pipeline_library.cpp
)

target_include_directories(rendering_engine
    PUBLIC include/
    PRIVATE src/
)

target_link_libraries(rendering_engine PUBLIC
    "-framework Metal"
    "-framework QuartzCore"
    "-framework Foundation"
)

# Shaders

# Compiling shader files
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/pbr.air
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
    COMMAND xcrun -sdk macosx metal
        -c ${CMAKE_SOURCE_DIR}/src/shaders/pbr.metal
        -o ${CMAKE_BINARY_DIR}/shaders/pbr.air
    DEPENDS ${CMAKE_SOURCE_DIR}/src/shaders/pbr.metal
    COMMENT "Compiling pbr.metal"
)

# Linking shaders together and outputting to default.metallib
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/default.metallib
    COMMAND xcrun -sdk macosx metallib
        ${CMAKE_BINARY_DIR}/shaders/pbr.air
        -o ${CMAKE_BINARY_DIR}/shaders/default.metallib
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/pbr.air
    COMMENT "Linking default.metallib"
)

# Putting the compiled binary (default.metallib) into .h file (default_metallib.h)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/default_metallib.h
    COMMAND xxd -i ${CMAKE_BINARY_DIR}/shaders/default.metallib |
            sed "s/unsigned char [a-zA-Z0-9_]*/unsigned char default_metallib/" |
            sed "s/unsigned int [a-zA-Z0-9_]*/unsigned int default_metallib_len/"
            > ${CMAKE_BINARY_DIR}/shaders/default_metallib.h
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/default.metallib
    COMMENT "Embedding metallib as header"
)

add_custom_target(shaders DEPENDS ${CMAKE_BINARY_DIR}/shaders/default_metallib.h)
add_dependencies(rendering_engine shaders)

# metal-cpp
target_include_directories(rendering_engine PRIVATE
    ${CMAKE_BINARY_DIR}/shaders
    external/metal-cpp
    external/stb
    external/ufbx
)

# Harness
option(BUILD_HARNESS "Build the rendering test harness" ON)

if(BUILD_HARNESS)
    add_subdirectory(harness)
endif()

enable_testing()
option(BUILD_TESTS "Build tests" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()