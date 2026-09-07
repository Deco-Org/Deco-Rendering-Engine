include (FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v3.15.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests 
    test.cpp
    transformation_system/transformation_system_test.cpp
    submesh_system/submesh_system_test.cpp
    material_system/texture_loader_test.cpp
    material_system/material_system_test.cpp
    metal_backend/render_pipeline_library_test.cpp
    metal_backend/shader_loader_test.cpp
    tools/synchronized_buffer_test.cpp
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/ufbx
    ${CMAKE_SOURCE_DIR}/external/metal-cpp
    ${CMAKE_SOURCE_DIR}/external/stb
    ${CMAKE_SOURCE_DIR}/tests/utils
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    rendering_engine
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_compile_options(tests PRIVATE
    -Werror=unused-variable
)

set_target_properties(tests PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    # XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# set_tests_properties(tests PROPERTIES
#     WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
# )