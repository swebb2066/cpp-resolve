
add_library(Util STATIC
  CppFile.cpp
  DirectoryEntryIterator.cpp
  Logger.cpp
)
target_compile_definitions(Util PRIVATE $<TARGET_PROPERTY:log4cxx,INTERFACE_COMPILE_DEFINITIONS> ${Boost_COMPILE_DEFINITIONS} BOOST_WAVE_STATIC_LINK)
target_include_directories(Util PUBLIC $<TARGET_PROPERTY:log4cxx,INTERFACE_INCLUDE_DIRECTORIES> ${Boost_INCLUDE_DIRS})
