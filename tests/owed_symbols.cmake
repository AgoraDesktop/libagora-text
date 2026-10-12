# THE OWED DOORS ARE DECLARED AND NOT DEFINED, and that is a claim a TEST can hold rather than a comment
# asserting it. A name a shared library DEFINES comes back from `nm -D --defined-only` with a type letter; a
# name that is merely declared does not come back at all. So this asks for ABSENCE -- which is the only shape
# the claim takes from a caller's side, and worth asking because a door that quietly acquired a definition
# would keep the comment and lose the property.
#
# usage: cmake -DNM=<nm> -DLIB=<library> "-DSYMBOLS=a;b" -P owed_symbols.cmake

if(NOT NM OR NOT LIB)
  message(FATAL_ERROR "owed_symbols.cmake needs -DNM and -DLIB")
endif()

execute_process(COMMAND "${NM}" -D --defined-only "${LIB}"
                OUTPUT_VARIABLE defined
                RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nm failed on ${LIB} (exit ${rc})")
endif()

set(offenders "")
foreach(sym IN LISTS SYMBOLS)
  if(defined MATCHES "(^|\n)[0-9a-fA-F]+ +[A-Za-z] ${sym}(\n|$)")
    list(APPEND offenders "${sym}")
  endif()
endforeach()

if(offenders)
  message(FATAL_ERROR "these doors are DECLARED AND OWED, and the library DEFINES them: ${offenders}")
endif()
message(STATUS "owed doors are absent, as the header says: ${SYMBOLS}")
