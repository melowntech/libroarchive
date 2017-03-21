# bump version here
set(roarchive_VERSION 1.0)

define_module(LIBRARY roarchive=${roarchive_VERSION}
  DEPENDS utility>=1.13
  Boost_FILESYSTEM
  MAGIC
  )

set(roarchive_SOURCES
  roarchive.hpp roarchive.cpp
  detail.hpp directory.cpp
  tarball.cpp zip.cpp
  )

add_library(roarchive STATIC
  ${roarchive_SOURCES}
  )
buildsys_library(roarchive)
target_link_libraries(roarchive ${MODULE_LIBRARIES})
buildsys_target_compile_definitions(roarchive ${MODULE_DEFINITIONS})
