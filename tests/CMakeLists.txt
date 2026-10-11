# libagora-text's own suite.
#
# It links the library and Core Foundation, because every door in the surface so far is spelled with Core
# Foundation types. Core Graphics is the library's own dependency and not the suite's.

add_executable(text text.c)
target_link_libraries(text PRIVATE Text CoreFoundation::CoreFoundation)

add_test(NAME text COMMAND text)
