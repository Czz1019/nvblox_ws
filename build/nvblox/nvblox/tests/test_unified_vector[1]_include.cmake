if(EXISTS "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector")
  if(NOT EXISTS "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector[1]_tests.cmake" OR
     NOT "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector[1]_tests.cmake" IS_NEWER_THAN "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector" OR
     NOT "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector[1]_tests.cmake" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_FILE}")
    include("/usr/share/cmake-3.22/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[/home/czz/nvblox_ws/src/nvblox/nvblox/tests]==]
      TEST_EXTRA_ARGS [==[]==]
      TEST_PROPERTIES [==[ENVIRONMENT;ASAN_OPTIONS=protect_shadow_gap=0]==]
      TEST_PREFIX [==[]==]
      TEST_SUFFIX [==[]==]
      TEST_FILTER [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[test_unified_vector_TESTS]==]
      CTEST_FILE [==[/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[30]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_unified_vector[1]_tests.cmake")
else()
  add_test(test_unified_vector_NOT_BUILT test_unified_vector_NOT_BUILT)
endif()
