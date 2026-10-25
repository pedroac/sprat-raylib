add_executable(sprite_sheet sprite_sheet.c)
target_link_libraries(sprite_sheet PRIVATE sprat-raylib::sprat-raylib)
if(NOT MSVC)
    target_compile_options(sprite_sheet PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(sprite_sheet PRIVATE m)
endif()

# The example finds resources/ next to its executable.
add_custom_command(TARGET sprite_sheet POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:sprite_sheet>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/sheet.sprat
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/atlas.png
        $<TARGET_FILE_DIR:sprite_sheet>/resources
)
