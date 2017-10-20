define_module(BINARY roarchive-tools
  DEPENDS roarchive service
  )

set(roarchive-cat_SOURCES
  cat.cpp
  )

add_executable(roarchive-cat ${roarchive-cat_SOURCES})
target_link_libraries(roarchive-cat ${MODULE_LIBRARIES})
buildsys_binary(roarchive-cat)
