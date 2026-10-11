#===----------------------------------------------------------------------===#
#
# Agora's build for Core Text (libagora-text).
#
# IMPORTED FROM FIWIX AND NOT YET CONVERTED — see PROVENANCE.md. The sources are Objective-C over
# FOUNDATION (18 of them import it) and they already sit on CORE GRAPHICS for their fonts and their
# geometry (`CGFontRef`, `CGPathRef`, `CGRect`). So this library needs three of its siblings:
#
#   libagora-core      Core Foundation, which the target is linked with once the conversion is made
#   libagora-base      Foundation, which the imported sources are written against today
#   libagora-drawing   Core Graphics, for the fonts and the geometry it already uses
#
# FreeType is the fourth thing it stands on, and it IS named here even though the fonts arrive as CGFontRef:
# four of these files #include <ft2build.h> themselves, and AN INCLUDE IS NOT SOMETHING A LINK DEPENDENCY
# CARRIES. The first version of this comment said the drawing library's link was enough, which is true for
# the linker and false for the preprocessor.
#
#===----------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.24)

project(AgoraText LANGUAGES C OBJC)

include(GNUInstallDirs)

set(CMAKE_POSITION_INDEPENDENT_CODE YES)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(CoreFoundation CONFIG REQUIRED)
find_package(Foundation CONFIG REQUIRED)
find_package(AgoraDrawing CONFIG REQUIRED)
find_package(Freetype REQUIRED)

# ---------------------------------------------------------------------------
# Include shim: these headers are included as <CoreText/...>, and CTFont.h
# includes its siblings the same way.
# ---------------------------------------------------------------------------
set(_agora_include_dir "${CMAKE_BINARY_DIR}/agora-include")
file(REMOVE_RECURSE "${_agora_include_dir}")
file(MAKE_DIRECTORY "${_agora_include_dir}")
file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}" "${_agora_include_dir}/CoreText" SYMBOLIC)

# ---------------------------------------------------------------------------
# The library: every .m and .c in the directory. THE OBJECTIVE-C FILES ARE NOT A
# SUBSET HERE, unlike libagora-core's dispatch translation units: this library is
# Objective-C by construction -- 14 of its 16 implementation files -- so the whole
# target is compiled with the tree's runtime and the cfstrings convention.
# ---------------------------------------------------------------------------
file(GLOB TEXT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.m"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(Text SHARED ${TEXT_SOURCES})

set_target_properties(Text PROPERTIES OUTPUT_NAME agora-text)

target_include_directories(Text
    PUBLIC
        "$<BUILD_INTERFACE:${_agora_include_dir}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")

# CORE FOUNDATION AND THE TWO SIBLINGS ARE PUBLIC, THE WAY THEY ARE IN BOTH OTHER LIBRARIES: a consumer of
# Core Text reaches all three itself -- CFStringRef for a format, NSAttributedString for the attributes, and
# CGFontRef and CGRect for what a line is laid out with. A PRIVATE link would fail at that consumer's first
# call, which is how exactly that was found when libagora-drawing got its first client.
target_link_libraries(Text
    PUBLIC
        CoreFoundation::CoreFoundation
        Foundation::Foundation
        AgoraDrawing::Drawing
    PRIVATE
        Freetype::Freetype
        objc)

target_compile_options(Text PRIVATE
    -fobjc-runtime=gnustep-2.0
    -fblocks
    -fconstant-cfstrings
    -Wno-objc-missing-super-calls
    -Wno-incomplete-implementation
    -Wno-objc-root-class)

# ---------------------------------------------------------------------------
# Install and export. THE EXPORT IS HERE FROM THE FIRST COMMIT, which is where
# libagora-drawing's should have been: it installed a library and a directory of
# headers and no package, so nothing could find_package it, and a client written
# later found the library's own Core Foundation link PRIVATE for the same reason.
# ---------------------------------------------------------------------------
install(DIRECTORY . DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CoreText
        FILES_MATCHING PATTERN "*.h")
install(TARGETS Text
    EXPORT AgoraTextTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)

install(EXPORT AgoraTextTargets
    FILE AgoraTextTargets.cmake
    NAMESPACE AgoraText::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AgoraText)

configure_package_config_file(cmake/AgoraTextConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AgoraTextConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AgoraText)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AgoraTextConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AgoraTextConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AgoraTextConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AgoraText)
