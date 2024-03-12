# === external dependencies ==================================================

# ARCHIE_BUILD_TESTING - user opiton to enable test buld along with regular build
option(ARCHIE_BUILD_TESTING "Extend builds with tests" OFF)
# ARCHIE_ERROR_FLAGS - variable (directory or parent scope) with default CXX options
if(NOT ARCHIE_ERROR_FLAGS)
  message(WARNING "ARCHIE_ERROR_FLAGS necessary for default CXX flags")
endif()

# === Coverage build =========================================================

if("Coverage" STREQUAL "${ARCHIE_BUILD_TYPE}") 
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH NAMES lcov lcov.bat lcov.exe lcov.perl)
    if(NOT GCOV_PATH OR NOT LCOV_PATH)
        message(FATAL_ERROR "ARCHIE: gcov and lcov is needed for coverage target")
    endif()
    # TODO(rishin): Support VC++
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_BUILD_TYPE MATCHES "Debug")
        # Debug option should set -O0 and -g flags necessary for coverage
        message(STATUS "ARCHIE: Building with coverage enabled")
    else()
        message(FATAL_ERROR "ARCHIE: Conflicting build options with coverage enabled")
    endif()

    add_custom_command(OUTPUT coverage.info
            COMMAND lcov --directory ${CMAKE_CURRENT_BINARY_DIR} --capture --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND lcov --list coverage.info
        )
    add_custom_target(coverage DEPENDS coverage.info)
endif()


# === Build functions ========================================================

macro(archie_cxx_deps_segregate shared_libs interface_libs)
  # TODO: remove this print
  #message(STATUS "shared_libs=${shared_libs} interface_libs=${interface_libs} deps_public=${ARGN}")
  set(deps_public ${ARGN})
  # Check whether a shared library is shared or interface
  set(${shared_libs} )
  set(${interface_libs} )
  foreach(dep ${deps_public})    
    get_target_property(dep_type ${dep} TYPE)
    if(dep_type STREQUAL "SHARED_LIBRARY")
      list(APPEND ${shared_libs} ${dep})
    elseif(dep_type STREQUAL "INTERFACE_LIBRARY")
      list(APPEND ${interface_libs} ${dep})
    else()
      message(FATAL_ERROR "${dep} needs to be shared or header libraries")
    endif()
  endforeach(dep)
endmacro()

# cc_shared_library(namespace target
#                   [SRCS ...] // c,cxx source files and private headers
#                   [INCL_PUBL ...]
#                   [DEPS_PRIV ...]
#                   [DEPS_PUBL ...]
#                   [COPTS]
# )
function(archie_cxx_library_shared namespace target)
  # Argument parsing, validation and defaults
  set(multiValueArgs SRCS INCL_PUBL DEPS_PRIV DEPS_PUBL COPTS)
  cmake_parse_arguments(CXX_LIB "" "" "${multiValueArgs}" ${ARGN})
  if(NOT CXX_LIB_SRCS)
    message(FATAL_ERROR "archie_cxx_shared_library needs SRCS parameters")
  endif()
  if(NOT CXX_LIB_COPTS)
    message(FATAL_ERROR "archie_cxx_shared_library needs COPTS parameters")
  endif()
  # Make shared library target
  set(lib_name "${namespace}-${target}")
  add_library(${lib_name} SHARED ${CXX_LIB_SRCS})
  target_sources(${lib_name} PRIVATE ${CXX_LIB_SRCS})
  target_compile_options(${lib_name} PRIVATE ${CXX_LIB_COPTS})
  # Coverage related flags
  # TODO(rishin): support VC++ also
  # TODO(rishin): Possibly move this to COPTS?
  if("Coverage" STREQUAL "${ARCHIE_BUILD_TYPE}")
    target_compile_options(${lib_name} PRIVATE --coverage)
    target_link_options("${lib_name}" PRIVATE --coverage)
  endif()
  # Add include directories  
  if(CXX_LIB_INCL_PUBL)
    target_include_directories(${lib_name} PUBLIC ${CXX_LIB_INCL_PUBL})
  endif()
  # Add private dependencies
  if(CXX_LIB_DEPS_PRIV)
    target_link_libraries(${lib_name} PRIVATE ${CXX_LIB_DEPS_PRIV})
  endif()
  # Add public dependencies
  archie_cxx_deps_segregate(shared_libs interface_libs ${CXX_LIB_DEPS_PUBL})
  if(shared_libs)
    target_link_libraries(${lib_name} PUBLIC ${shared_libs})
  endif()
  if(interface_libs)
    target_link_libraries(${lib_name} INTERFACE ${interface_libs})
  endif()
  # Alias the library to have a namespace
  add_library(${namespace}::${target} ALIAS ${lib_name})
endfunction()

# cc_shared_library(namespace target
#                   [SRCS ...]
#                   [INCL_PUBL ...]
#                   [DEPS_PUBL ...]
#                   [COPTS]
# )
function(archie_cxx_library_header namespace target)
  # Argument parsing, validation and defaults
  set(multiValueArgs SRCS INCL_PUBL DEPS_PUBL COPTS)
  cmake_parse_arguments(CXX_HDR "" "" "${multiValueArgs}" ${ARGN})
  if(NOT CXX_HDR_SRCS)
    message(FATAL_ERROR "archie_cxx_header_library needs SRCS parameters")
  endif()
  if(NOT CXX_HDR_COPTS)
    message(FATAL_ERROR "archie_cxx_header_library needs COPTS parameters")
  endif()
  # Make header library targets
  set(lib_name "${namespace}-${target}")
  add_library(${lib_name} INTERFACE)
  target_sources(${lib_name} INTERFACE ${CXX_HDR_SRCS})
  target_compile_options(${lib_name} INTERFACE ${CXX_HDR_COPTS})
  # add public header deps
  if(CXX_HDR_INCL_PUBL)
    target_include_directories(${lib_name} INTERFACE ${CXX_HDR_INCL_PUBL})
  endif()
  # Add public dependencies
  archie_cxx_deps_segregate(shared_libs interface_libs ${CXX_HDR_DEPS_PUBL})
  if(shared_libs)
    target_link_libraries(${lib_name} PUBLIC ${shared_libs})
  endif()
  if(interface_libs)
    target_link_libraries(${lib_name} INTERFACE ${interface_libs})
  endif()
  # Alias the library to have a namespace
  add_library(${namespace}::${target} ALIAS ${lib_name})
endfunction()


# cc_binary(namespace target
#           [SRCS ...]
#           [DEPS_PRIV ...]
#           [DEPS_PUBL ...]
#           [COPTS]
#           [EXCLUDE_FROM_ALL]
# )
function(archie_cxx_executable namespace target)
  # Argument parsing, validation and defaults
  set(options EXCLUDE_FROM_ALL)
  set(multiValueArgs SRCS DEPS_PRIV DEPS_PUBL COPTS)
  cmake_parse_arguments(CXX_EXE "${options}" "" "${multiValueArgs}" ${ARGN})
  if(NOT CXX_EXE_SRCS)
    message(FATAL_ERROR "archie_cxx_executable needs SRCS parameter")
  endif()
  if(NOT CXX_EXE_COPTS)
    message(FATAL_ERROR "archie_cxx_shared_library needs COPTS parameters")
  endif()
  # Make executable target
  set(exec_name "${namespace}-${target}")
  if(NOT CXX_EXE_EXCLUDE_FROM_ALL)
    add_executable(${exec_name} ${CXX_EXE_SRCS})
  else()
    add_executable(${exec_name} EXCLUDE_FROM_ALL ${CXX_EXE_SRCS})
  endif()
  target_compile_options(${exec_name} PRIVATE ${CXX_EXE_COPTS})
  # Add private dependencies
  if(CXX_EXE_DEPS_PRIV)
    target_link_libraries(${exec_name} PRIVATE ${CXX_EXE_DEPS_PRIV})
  endif()
  # Add public dependencies
  archie_cxx_deps_segregate(shared_libs interface_libs ${CXX_EXE_DEPS_PUBL})
  if(shared_libs)
    target_link_libraries(${exec_name} PUBLIC ${shared_libs})
  endif()
  if(interface_libs)
    target_link_libraries(${exec_name} INTERFACE ${interface_libs})
  endif()
endfunction()
 


# TODO(rishin): Serach for installed gtest before downloading
if(ARCHIE_ENABLE_TESTING)
  find_package(GTest REQUIRED)

#FetchContent_Declare(
#    googletest
#    GIT_REPOSITORY    "https://github.com/google/googletest"
#    GIT_TAG           "release-1.8.0"
#    GIT_SHALLOW       ON
#  )
#  # TODO(rishin): Change this to FetchContent_MakeAvailable and update cmake version to 3.14
#  FetchContent_GetProperties(googletest)
#  if(NOT googletest_POPULATED)
#      FetchContent_Populate(googletest)
#      # add the targets: gtest,gtest_main,gmock,gmock_main
#      add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
#  endif()

  include(CTest)

  # TODO(rishin): gtest should be scoped properly
  # TODO(rishin): use inbuilt gtest function instead of archite. See https://cmake.org/cmake/help/latest/module/GoogleTest.html#command:gtest_add_tests
  function(archie_cxx_test namespace target)
    if(NOT TARGET test-build)
        add_custom_target(test-build)
    endif()
    archie_cxx_executable(${namespace} ${target} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries("${namespace}-${target}" PRIVATE GTest::Main)
    add_dependencies(test-build "${namespace}-${target}")
    gtest_add_tests(TARGET "${namespace}-${target}"
                    TEST_PREFIX "${namespace}:")
  endfunction()

endif()
