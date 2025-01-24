cmake_minimum_required(VERSION 3.14)

# set the project name
project(TFlies)

find_package(yaml-cpp REQUIRED)
find_package(GTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/include/)
include_directories(${PROJECT_SOURCE_DIR}/third_lib/)

add_definitions("-Wall -g -std=c++17")

# add the executable
add_executable(TFlies
    src/logger.cpp
    src/TFlies.cpp
    src/id.cpp
    src/tNode.cpp)
# add_executable(main 
#     src/main.cpp 
#     src/Controller.cpp
#     src/Model.cpp
#     src/View.cpp
#     src/task.cpp
#     src/ui.cpp
#     utils/utils.cpp)

target_link_libraries(TFlies PUBLIC sqlite3 yaml-cpp)

#test 
# add_executable(id_test test/ncurses_test.cpp)
# target_link_libraries(ncurses_test PUBLIC ncursesw panelw)


include(GoogleTest)
enable_testing()
add_executable(id_test
  src/logger.cpp
  src/id.cpp
  test/id_test.cpp
)

target_link_libraries(
  id_test
  GTest::gtest_main
)

gtest_discover_tests(id_test)
