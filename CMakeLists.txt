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
    src/tools/range_allocator.cpp
    src/asset_systems/texture_loader.cpp
    src/asset_systems/material_system.cpp
    src/asset_systems/submesh_system.cpp
    src/asset_systems/animation_importer.cpp
    src/core_systems/transformation_system.cpp
    src/core_systems/animation_system.cpp
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

# metal-cpp
target_include_directories(rendering_engine PRIVATE
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