add_executable(sandbox main.cpp)
target_include_directories(sandbox PUBLIC ../inc)
target_link_libraries(sandbox pthread)
