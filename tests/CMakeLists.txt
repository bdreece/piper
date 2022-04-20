find_package(Boost COMPONENTS unit_test_framework)

if(${Boost_FOUND})
  enable_testing()

  add_executable(mpsc mpsc.cpp)
  target_include_directories(mpsc PUBLIC ../inc)
  target_link_libraries(mpsc pthread ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  add_test(NAME mpsc COMMAND mpsc --logger=HRF,message,mpsc.log -r detailed)

  add_executable(spmc spmc.cpp)
  target_include_directories(spmc PUBLIC ../inc)
  target_link_libraries(spmc pthread ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  add_test(NAME spmc COMMAND spmc --logger=HRF,message,spmc.log -r detailed)
endif()
