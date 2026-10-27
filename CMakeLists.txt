cmake_minimum_required(VERSION 3.24)

# VERSION holds "vMAJOR.MINOR.PATCH", like sprat-lib.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" SPRAT_RAYLIB_VERSION_TAG)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
string(STRIP "${SPRAT_RAYLIB_VERSION_TAG}" SPRAT_RAYLIB_VERSION_TAG)
string(REGEX REPLACE "^v" "" SPRAT_RAYLIB_VERSION "${SPRAT_RAYLIB_VERSION_TAG}")

project(sprat-raylib VERSION ${SPRAT_RAYLIB_VERSION} LANGUAGES C)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SPRAT_RAYLIB_BUILD_EXAMPLES "Build the example programs" ${PROJECT_IS_TOP_LEVEL})
option(SPRAT_RAYLIB_SANITIZE "Build with AddressSanitizer and UBSan (GCC/Clang)" OFF)

if(SPRAT_RAYLIB_SANITIZE AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Dependencies: an installed package when there is one, otherwise a download.
# To build against a local checkout, pass -DFETCHCONTENT_SOURCE_DIR_SPRAT=../sprat-lib
# (or FETCHCONTENT_SOURCE_DIR_RAYLIB).
include(FetchContent)
if(NOT TARGET raylib)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
        FIND_PACKAGE_ARGS 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()
if(NOT TARGET sprat::sprat)
    FetchContent_Declare(sprat
        GIT_REPOSITORY https://github.com/pedroac/sprat-lib.git
        GIT_TAG 97d87dba91f20dcbe9c2c45b2bdb749ccaa9a849   # sprat-lib main, v0.7.0
        FIND_PACKAGE_ARGS 0.7
    )
    FetchContent_MakeAvailable(sprat)
endif()

add_library(sprat-raylib src/sprat-raylib.c)
add_library(sprat-raylib::sprat-raylib ALIAS sprat-raylib)
target_include_directories(sprat-raylib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(sprat-raylib PUBLIC raylib PRIVATE sprat::sprat)
set_target_properties(sprat-raylib PROPERTIES PUBLIC_HEADER src/sprat-raylib.h)
if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(sprat-raylib PRIVATE SPRAT_RAYLIB_BUILD_SHARED INTERFACE SPRAT_RAYLIB_USE_SHARED)
endif()

if(MSVC)
    target_compile_options(sprat-raylib PRIVATE /W4)
else()
    target_compile_options(sprat-raylib PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

if(SPRAT_RAYLIB_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CTest)
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(tests)
endif()

# Installing exports the target for find_package(), which only works when raylib and sprat
# are installed packages too: a downloaded raylib is in no export set.
get_target_property(SPRAT_RAYLIB_RAYLIB_IMPORTED raylib IMPORTED)
get_target_property(SPRAT_RAYLIB_SPRAT_IMPORTED sprat::sprat IMPORTED)
if(SPRAT_RAYLIB_RAYLIB_IMPORTED AND SPRAT_RAYLIB_SPRAT_IMPORTED)
    set(SPRAT_RAYLIB_INSTALL_DEFAULT ON)
else()
    set(SPRAT_RAYLIB_INSTALL_DEFAULT OFF)
endif()
option(SPRAT_RAYLIB_INSTALL "Generate install rules (needs installed raylib and sprat packages)" ${SPRAT_RAYLIB_INSTALL_DEFAULT})

if(SPRAT_RAYLIB_INSTALL)
    install(TARGETS sprat-raylib EXPORT sprat-raylib-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(EXPORT sprat-raylib-targets
        NAMESPACE sprat-raylib::
        FILE sprat-raylib-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sprat-raylib
    )
    include(CMakePackageConfigHelpers)
    configure_package_config_file(cmake/sprat-raylib-config.cmake.in
        ${PROJECT_BINARY_DIR}/sprat-raylib-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sprat-raylib
    )
    write_basic_package_version_file(
        ${PROJECT_BINARY_DIR}/sprat-raylib-config-version.cmake
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        ${PROJECT_BINARY_DIR}/sprat-raylib-config.cmake
        ${PROJECT_BINARY_DIR}/sprat-raylib-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sprat-raylib
    )
endif()
