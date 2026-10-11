# libagora-text's own suite.
#
# It links the library and Core Foundation, because every door here is spelled with Core Foundation types.
# Core Graphics is the library's own dependency and not the suite's.

add_executable(CTFontTraits CTFontTraits.c)
target_link_libraries(CTFontTraits PRIVATE Text CoreFoundation::CoreFoundation)

add_test(NAME CTFontTraits COMMAND CTFontTraits)
