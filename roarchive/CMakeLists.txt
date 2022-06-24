
# bump version here
set(roarchive_VERSION 1.8)

set(roarchive_EXTRA_SOURCES)
set(roarchive_EXTRA_DEPENDS)
set(roarchive_DEFINITIONS)
if(MODULE_http_FOUND)
  message(STATUS "roarchive: compiling in http support")
  list(APPEND roarchive_EXTRA_DEPENDS http>=1.8)
  list(APPEND roarchive_EXTRA_SOURCES http.cpp)
  list(APPEND roarchive_DEFINITIONS ROARCHIVE_HAS_HTTP=1)
else()
  message(STATUS "roarchive: compiling without http support")
endif()

define_module(LIBRARY roarchive=${roarchive_VERSION}
  DEPENDS ${roarchive_EXTRA_DEPENDS} utility>=1.31
  Boost_FILESYSTEM Boost_IOSTREAMS
  MAGIC
  DEFINITIONS ${roarchive_DEFINITIONS}
  )

set(roarchive_SOURCES
  io.hpp
  istream.hpp
  error.hpp
  roarchive.hpp roarchive.cpp detail.hpp
  directory.cpp tarball.cpp zip.cpp
  ${roarchive_EXTRA_SOURCES}
  )

add_library(roarchive STATIC
  ${roarchive_SOURCES}
  ${roarchive_ZIP_SOURCES}
  )
buildsys_library(roarchive)
target_link_libraries(roarchive ${MODULE_LIBRARIES})
target_compile_definitions(roarchive PRIVATE ${MODULE_DEFINITIONS})

add_subdirectory(test-roarchive EXCLUDE_FROM_ALL)

if(MODULE_service_FOUND)
  add_subdirectory(tools EXCLUDE_FROM_ALL)
endif()
