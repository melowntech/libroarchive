
# bump version here
set(roarchive_VERSION 1.1)

define_module(LIBRARY roarchive=${roarchive_VERSION}
  DEPENDS utility>=1.15
  Boost_FILESYSTEM Boost_IOSTREAMS
  MAGIC
  )

set(roarchive_SOURCES
  roarchive.hpp roarchive.cpp
  detail.hpp directory.cpp
  tarball.cpp zip.cpp
  )

add_library(roarchive STATIC
  ${roarchive_SOURCES}
  ${roarchive_ZIP_SOURCES}
  )
buildsys_library(roarchive)
target_link_libraries(roarchive ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(roarchive ${MODULE_DEFINITIONS})

add_subdirectory(test-roarchive EXCLUDE_FROM_ALL)
