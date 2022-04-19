find_package(Boost COMPONENTS unit_test_framework)

if(${Boost_FOUND})
  enable_testing()

  add_executable(mpsc mpsc.cpp)
  target_include_directories(mpsc PUBLIC ../inc)
  target_link_libraries(mpsc ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  add_test(NAME mpsc COMMAND mpsc --logger=HRF,message,mpsc.log -r detailed)
endif()
