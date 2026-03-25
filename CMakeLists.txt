cmake_minimum_required(VERSION 3.13)
project(cpp_resolve)
include(CTest)
if(EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
  include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

set(Boost_USE_STATIC_LIBS  ON)
find_package(Boost COMPONENTS wave filesystem program_options unit_test_framework REQUIRED )
message("-- Found Boost ${Boost_INCLUDE_DIR}")
if(WIN32)
  get_target_property(Boost_COMPILE_DEFINITIONS
    Boost::disable_autolinking INTERFACE_COMPILE_DEFINITIONS
  )
endif()
find_package(log4cxx 1.6 REQUIRED )
get_target_property(LOG4CXX_DLL_PATH log4cxx IMPORTED_LOCATION_RELEASE)
get_filename_component(LOG4CXX_FILE_DIR "${LOG4CXX_DLL_PATH}" PATH)
message("-- Found log4cxx ${LOG4CXX_FILE_DIR}")

# Add support for linking statically
option(BUILD_SHARED_LIBS "Use dynamically linked runtime" ON)
if(WIN32 AND NOT BUILD_SHARED_LIBS)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
  endforeach()
endif()

# Building
add_subdirectory(util)
add_executable(cpp_resolve
  cpp_resolve.cpp 
)
target_compile_definitions(cpp_resolve PRIVATE ${Boost_COMPILE_DEFINITIONS} BOOST_WAVE_STATIC_LINK BOOST_WAVE_SUPPORT_LONGLONG_INTEGER_LITERALS=1)
target_include_directories(cpp_resolve PRIVATE $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES> ${Boost_INCLUDE_DIRS})
target_link_libraries(cpp_resolve PRIVATE Util log4cxx ${Boost_LIBRARIES})

# Testing
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

# Installation
include(GNUInstallDirs)
set(TARGET_EXECUTABLES "$<TARGET_FILE:cpp_resolve>" )
install(CODE [[
  file(GET_RUNTIME_DEPENDENCIES
    RESOLVED_DEPENDENCIES_VAR cpp_resolve_dependencies
    UNRESOLVED_DEPENDENCIES_VAR cpp_resolve_unresolved
    POST_EXCLUDE_REGEXES ^/lib ^/usr/lib ^/usr/local/lib
    EXECUTABLES ${TARGET_EXECUTABLES}
    )
  file(INSTALL ${cpp_resolve_dependencies}
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
    FOLLOW_SYMLINK_CHAIN
  )
]])
install(TARGETS cpp_resolve
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
if(MSVC)
  get_target_property(LOG4CXX_RUNTIME_LIBRARY log4cxx IMPORTED_LOCATION_RELEASE)
  INSTALL(FILES "${LOG4CXX_RUNTIME_LIBRARY}"
          DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  )
  INSTALL(FILES $<TARGET_PDB_FILE:cpp_resolve>
          DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
          CONFIGURATIONS RelWithDebInfo Debug
  )
endif(MSVC)
install(FILES  "${CMAKE_CURRENT_SOURCE_DIR}/resources/cpp_resolve.properties"
  DESTINATION   ${CMAKE_INSTALL_PREFIX}/bin
)

