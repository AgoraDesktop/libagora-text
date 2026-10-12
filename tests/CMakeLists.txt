# libagora-text's own suite.
#
# IT LINKS THE LIBRARY AND CORE FOUNDATION, AND REACHES CORE GRAPHICS THROUGH THE LIBRARY: every door in the
# surface so far is spelled with Core Foundation types, and the two Core Graphics things this suite needs
# itself -- `CGDataProviderCreateWithFilename` with `CGFontCreateWithDataProvider`, which is how a face is
# loaded from a file, and the engine seam under them -- come in on `Text`'s PUBLIC link rather than by a
# second `find_package`. (This file said "Core Graphics is the library's own dependency and not the suite's",
# which was never true of this suite: `case_font_object` has loaded a face from a file since it was written.)

add_executable(text text.c)
target_link_libraries(text PRIVATE Text CoreFoundation::CoreFoundation)

add_test(NAME text COMMAND text)

# AND `ctest` HAS TO BE THE WHOLE COMMAND, which is why the loader's search path is set HERE rather than left
# to the caller's shell. THE LOADER NEEDS IT FOR THE TRANSITIVE CASE: `libagora-core.so` records no runtime
# path of its own, and the one library only IT needs -- `libobjc.so.4.6` -- is therefore found through the
# search path or not at all. `DT_RUNPATH` is not consulted for a dependency of a dependency, so no rpath on
# this executable can cover it, and a bare `ctest` would fail with "cannot open shared object file" after the
# suite had already been built cleanly. THE DIRECTORIES ARE TAKEN FROM THE IMPORTED TARGETS rather than
# written down, so this cannot disagree with the prefix the library was actually built against.
set(_agora_runtime_dirs
    "$<TARGET_FILE_DIR:CoreFoundation::CoreFoundation>"
    "$<TARGET_FILE_DIR:AgoraDrawing::Drawing>")
list(REMOVE_DUPLICATES _agora_runtime_dirs)
string(REPLACE ";" ":" _agora_ld_path "${_agora_runtime_dirs}")
set_tests_properties(text PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${_agora_ld_path}")

# AND THE OWED DOORS ARE HELD BY MEASUREMENT RATHER THAN BY THE COMMENT THAT SAYS SO. CTFontCollection.c states
# that its two matching doors are declared and NOT defined; a comment asserting that is worth little, because a
# door that quietly acquired a definition elsewhere would keep the comment and lose the property. `nm` is the
# instrument and tests/owed_symbols.cmake reads it — and the list below is the one place a newly-owed door has
# to be named, beside the reason its own file carries.
#
# AND THE LIST WAS INCOMPLETE, WHICH IS A FIX AND NOT A TIDY-UP: it named ONE door while TWENTY-THREE were declared
# and undefined, because it was written when the `CTFontManager` doors were owed and never picked up the older `CTFont`
# tier's. THE CHECK AND THE ACCOUNTING DISAGREED, and the accounting was the honest one: `CTFont.c` has carried an owed
# block naming all twenty-two of its own with a reason each. What was missing was the GUARD — a door that quietly
# acquired a definition would have passed this test without a word. The list is now every door whose header declares it
# and whose library does not define it, measured by comparing Apple's headers against `nm -D` on the built library.
find_program(AGORA_NM NAMES nm llvm-nm REQUIRED)
add_test(NAME owed
    COMMAND "${CMAKE_COMMAND}"
            "-DNM=${AGORA_NM}"
            "-DLIB=$<TARGET_FILE:Text>"
            "-DSYMBOLS=CTFontCopyFeatures;CTFontCopyFeatureSettings;CTFontCopySupportedLanguages;CTFontCopyTraits;CTFontCopyVariationAxes;CTFontCreateCopyWithFamily;CTFontCreateCopyWithSymbolicTraits;CTFontCreateForString;CTFontCreateUIFontForLanguage;CTFontCreateWithPlatformFont;CTFontDescriptorCreateCopyWithFeature;CTFontGetPlatformFont;CTFontGetSlantAngle;CTFontGetStringEncoding;CTFontGetVerticalTranslationsForGlyphs;CTFontManagerCreateFontRequestRunLoopSource"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/owed_symbols.cmake")
