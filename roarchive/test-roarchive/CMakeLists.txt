define_module(BINARY test-roarchive
  DEPENDS roarchive
)

add_executable(test-roarchive test-roarchive.cpp)
target_link_libraries(test-roarchive ${MODULE_LIBRARIES})
target_compile_definitions(test-roarchive PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(test-roarchive)

add_executable(roarchive-zcat roarchive-zcat.cpp)
target_link_libraries(roarchive-zcat ${MODULE_LIBRARIES})
target_compile_definitions(roarchive-zcat PRIVATE ${MODULE_DEFINITIONS})
buildsys_binary(roarchive-zcat)
