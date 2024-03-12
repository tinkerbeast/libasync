# === Define defaults ========================================================

# Create Archie default build flags

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # error options
    # See https://stackoverflow.com/questions/5088460/flags-to-enable-thorough-and-verbose-g-warnings
    # See https://stackoverflow.com/questions/399850/best-compiler-warning-level-for-c-c-compilers/401276#401276
    set(ARCHIE_ERROR_FLAGS -pedantic -pedantic-errors -Wall -Werror -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wextra -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline -Winvalid-pch -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wnon-virtual-dtor -Wpacked -Wpointer-arith -Wredundant-decls -Wsign-conversion -Wstack-protector -Wstrict-aliasing=2 -Wunreachable-code -Wunused -Wunused-parameter -Wvariadic-macros -Wwrite-strings -Wswitch-enum -Wdeprecated-copy-dtor -Wunused-value -Wshadow)
    # perf related
    set(ARCHIE_DEBUGABILITY_FLAGS -fno-omit-frame-pointer)
    # security related
    # TODO(rishin): Make fortify source level 2 work cmake
    # TODO(rishin): -DCMAKE_POSITION_INDEPENDENT_CODE
    set(ARCHIE_SECURITY_FLAGS -fPIE -fPIC -D_FORTIFY_SOURCE=1)
    set(ARCHIE_CXX_COPTS "${ARCHIE_ERROR_FLAGS};${ARCHIE_DEBUGABILITY_FLAGS};${ARCHIE_SECURITY_FLAGS}")
else()
  mesage(ERROR "Currently unspported compiler type")
# TODO(rishin): add flags for Clang
# TODO(rishin): add flags for VC++
endif()

# Archie default environment discovery flags

find_program(CCACHE_FOUND ccache) # Integration with ccahe.
if(CCACHE_FOUND)
    set(ARCHIE_COMPILER_CACHE ccache)
    set(ARCHIE_LINKER_CACHE ccache)
endif(CCACHE_FOUND)


# === Default includes =======================================================

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)


# === Set Archie default behaviours (current directory and subdir scope unless overwritten) =

# Needed for third-party tools like vim and mull (C++ mutator)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

# ARCHIE_BUILD_TYPE overrides CMAKE_BUILD_TYPE. It also provides additional 
# build types like "Coverage"". If ARCHIE_BUILD_TYPE is not set, it takes the 
# value from CMAKE_BUILD_TYPE. Default ARCHIE_BUILD_TYPE is "Debug".
if(NOT ARCHIE_BUILD_TYPE)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
  endif()
  set(ARCHIE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
else()
  if ("Coverage" STREQUAL "${ARCHIE_BUILD_TYPE}")
    set(CMAKE_BUILD_TYPE "Debug")
  else()
    set(CMAKE_BUILD_TYPE "${ARCHIE_BUILD_TYPE}")
  endif()
endif()
