#===----------------------------------------------------------------------===#
#
# Agora's build for Core Text (libagora-text).
#
# CORE TEXT, WRITTEN FROM SCRATCH AGAINST APPLE'S 10.6 HEADERS (user, 2026-10-11). The Fiwix import that
# used to be here is deleted -- it was Objective-C over Foundation, and it is still in this repository's
# history (`d14dc67`) if anything in it is ever wanted as a reference.
#
# TWO DEPENDENCIES, WHICH ARE APPLE'S OWN TWO: Core Foundation (libagora-core) and Core Graphics
# (libagora-drawing). NOT FOUNDATION, AND NOT OBJECTIVE-C. Apple's Core Text has no classes at all --
# `CTFontRef` is a typealias and its doors are C functions, measured from Apple's own index by
# ../../Fiwix/tools/coretext-sweep.py -- so this target is `LANGUAGES C` and every implementation file is a
# `.c`. The import was `LANGUAGES C OBJC` with fourteen `.m` files; there will be none.
#
# THE GUIDE: Apple's 10.6 headers are downloaded to /tmp/apple-sdk106/ (16 CoreText, 8 ATS) by the recipe
# in PROVENANCE.md, and Apple's documented surface is the ledger at
# ../../Fiwix/docs/reference/coretext-apple-surface.txt -- 2032 rows with a type and a status each.
# THE LEDGER IS THE ROW LIST AND NOT A MEASUREMENT OF THIS LIBRARY, which is a correction: the sweep it came
# with computes its verdict over the FIWIX TREE's own userland/CoreText -- the Objective-C import THIS
# repository deleted -- so every row for a type this library has not written yet still reads `shipped` there.
# What measures this library is its own suite and `nm -D` on its own library (see README.md). THE HEADERS ARE
# NOT IN THIS REPOSITORY
# AND MUST NOT BE: the tree's own ledger says the SDK "is not redistributable", so what is committed is
# the recipe and the declarations written from it.
#
#===----------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.24)

project(AgoraText LANGUAGES C)

include(GNUInstallDirs)

set(CMAKE_POSITION_INDEPENDENT_CODE YES)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(CoreFoundation CONFIG REQUIRED)
find_package(AgoraDrawing CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Include shim: these headers are included as <CoreText/...>, which is how
# Apple's own include each other and how a consumer writes them.
# ---------------------------------------------------------------------------
set(_agora_include_dir "${CMAKE_BINARY_DIR}/agora-include")
file(REMOVE_RECURSE "${_agora_include_dir}")
file(MAKE_DIRECTORY "${_agora_include_dir}")
file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}" "${_agora_include_dir}/CoreText" SYMBOLIC)

# ---------------------------------------------------------------------------
# The library: every .c in the directory. THERE ARE NONE YET, and that is a
# state rather than an accident: the surface is being written in Apple's own
# include order -- CTFontTraits, CTFontDescriptor, CTFont, then the layout
# headers above them -- and the first implementation file arrives with the first
# type that has behaviour.
# ---------------------------------------------------------------------------
file(GLOB TEXT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

if(NOT TEXT_SOURCES)
  message(FATAL_ERROR
    "libagora-text has no implementation files yet: the public surface is being written first, in the "
    "order Apple's headers include each other. See README.md for where that has got to.")
endif()

add_library(Text SHARED ${TEXT_SOURCES})

set_target_properties(Text PROPERTIES OUTPUT_NAME agora-text)

# -fconstant-cfstrings, so a CFSTR in a .c file is a constant rather than a call -- the same flag
# libagora-drawing compiles its own C sources with. -fblocks arrives on the INTERFACE of AgoraDrawing, whose
# Core Foundation headers need it, so this target gets it from the link rather than declaring it twice.
target_compile_options(Text PRIVATE -fconstant-cfstrings)

target_include_directories(Text
    PUBLIC
        "$<BUILD_INTERFACE:${_agora_include_dir}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")

# CORE FOUNDATION AND CORE GRAPHICS ARE PUBLIC, BECAUSE A CONSUMER OF CORE TEXT REACHES BOTH ITSELF:
# CFStringRef for a name, CFAttributedStringRef for the text being laid out, CGFont/CGSize/CGRect for what
# a line is measured and drawn with. A PRIVATE link would fail at that consumer's first call -- the reason
# libagora-drawing's own Core Foundation link had to be made public when it got its first client.
target_link_libraries(Text
    PUBLIC
        CoreFoundation::CoreFoundation
        AgoraDrawing::Drawing)

enable_testing()
add_subdirectory(tests)

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
