project(cppasync-Coroutine
    DESCRIPTION "C++20 coroutine library"
    LANGUAGES CXX)
cmake_minimum_required(VERSION 3.13.5)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include archie CMake libraries
include(archie-defaults) # Should only be included in top-level cmake file
include(archie-builds)



# Set global properties (WARNING: Only to be done at top level CMake file)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${ARCHIE_COMPILER_CACHE})
#set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${ARCHIE_LINKER_CACHE}) # linker cache disabled because of no major speed improvements


# === Set directory scope variables which affect target properties ===========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# === Adding individual libraries =============================================

# **TODO**: -fcoroutines doesn't work with switch default warning and all warnings as errors
#         : so they had to be disabled
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(ARCHIE_CXX_FLAGS_COROUTINE -fcoroutines)
else()
  mesage(ERROR "Currently unspported compiler type for COROUTINES")
# TODO(rishin): add flags for Clang
# TODO(rishin): add flags for VC++
endif()


find_package(FMT REQUIRED)
archie_cxx_library_header("async" "debug_log"
  SRCS      ${CMAKE_CURRENT_SOURCE_DIR}/include/async/debug_log.h
  INCL_PUBL ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DEPS_PUBL fmt::fmt-header-only
  COPTS     ${ARCHIE_CXX_COPTS}
)

archie_cxx_library_header("async" "concept"
  SRCS      ${CMAKE_CURRENT_SOURCE_DIR}/include/async/concept/awaiter.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/async/concept/promise.h
  INCL_PUBL ${CMAKE_CURRENT_SOURCE_DIR}/include/  
  COPTS     ${ARCHIE_CXX_COPTS}
)

archie_cxx_library_header("async" "coro"
  SRCS      ${CMAKE_CURRENT_SOURCE_DIR}/include/async/coro.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/async/polymorphic_awaiter.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/async/runner.h
  INCL_PUBL ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DEPS_PUBL async::concept async::debug_log
  COPTS     ${ARCHIE_CXX_COPTS} ${ARCHIE_CXX_FLAGS_COROUTINE}
)

archie_cxx_executable("async" "main"
  SRCS      ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  DEPS_PRIV async::coro
  COPTS     ${ARCHIE_CXX_COPTS} ${ARCHIE_CXX_FLAGS_COROUTINE}
)


if(ARCHIE_ENABLE_TESTING)
  archie_cxx_test("async" "test_coro"
    SRCS      ${CMAKE_CURRENT_SOURCE_DIR}/test/coro_test.cpp
    DEPS_PRIV async::coro
    COPTS     ${ARCHIE_CXX_COPTS} ${ARCHIE_CXX_FLAGS_COROUTINE}
  )
endif()