include (FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v3.15.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests 
    test.cpp
    mesh/mesh_test.cpp
)

target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/ufbx
    ${CMAKE_SOURCE_DIR}/external/metal-cpp
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    rendering_engine
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

set_target_properties(tests PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    # XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# set_tests_properties(tests PROPERTIES
#     WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
# )