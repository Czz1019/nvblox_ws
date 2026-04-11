if(EXISTS "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view")
  if(NOT EXISTS "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view[1]_tests.cmake" OR
     NOT "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view[1]_tests.cmake" IS_NEWER_THAN "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view" OR
     NOT "/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view[1]_tests.cmake" IS_NEWER_THAN "${CMAKE_CURRENT_LIST_FILE}")
    include("/usr/share/cmake-3.22/Modules/GoogleTestAddTests.cmake")
    gtest_discover_tests_impl(
      TEST_EXECUTABLE [==[/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view]==]
      TEST_EXECUTOR [==[]==]
      TEST_WORKING_DIR [==[/home/czz/nvblox_ws/src/nvblox/nvblox/tests]==]
      TEST_EXTRA_ARGS [==[]==]
      TEST_PROPERTIES [==[ENVIRONMENT;ASAN_OPTIONS=protect_shadow_gap=0]==]
      TEST_PREFIX [==[]==]
      TEST_SUFFIX [==[]==]
      TEST_FILTER [==[]==]
      NO_PRETTY_TYPES [==[FALSE]==]
      NO_PRETTY_VALUES [==[FALSE]==]
      TEST_LIST [==[test_gpu_layer_view_TESTS]==]
      CTEST_FILE [==[/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view[1]_tests.cmake]==]
      TEST_DISCOVERY_TIMEOUT [==[30]==]
      TEST_XML_OUTPUT_DIR [==[]==]
    )
  endif()
  include("/home/czz/nvblox_ws/build/nvblox/nvblox/tests/test_gpu_layer_view[1]_tests.cmake")
else()
  add_test(test_gpu_layer_view_NOT_BUILT test_gpu_layer_view_NOT_BUILT)
endif()
