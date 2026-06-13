add_executable(rendering_engine_harness main.mm)

target_link_libraries(rendering_engine_harness PRIVATE
    rendering_engine
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework Metal"
)

set_source_files_properties(
    ../src/shaders/light.metal
    ../src/shaders/sphere.metal
    PROPERTIES
    LANGUAGE Metal
    COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/src"
)

target_sources(rendering_engine_harness PRIVATE
    ../src/shaders/light.metal
    ../src/shaders/sphere.metal
)

set_target_properties(rendering_engine_harness PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
)

# copying assets to be next to the harness directory
# add_custom_command(TARGET harness POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_SOURCE_DIR}/assets
#     $<TARGET_FILE_DIR:harness>/assets
# )