cmake_minimum_required(VERSION 3.10)
project(utils_tests)

# 手动指定 GTest 路径
find_library(GTEST_LIBRARY gtest PATHS /usr/lib/x86_64-linux-gnu)
find_library(GTEST_MAIN_LIBRARY gtest_main PATHS /usr/lib/x86_64-linux-gnu)
find_path(GTEST_INCLUDE_DIR gtest/gtest.h PATHS /usr/include)

if(NOT GTEST_LIBRARY OR NOT GTEST_MAIN_LIBRARY)
    message(FATAL_ERROR "GTest not found")
endif()

set(INCLUDE_DIRS ../include)

set(DUMP_ANALYZER_SOURCES ../src/utils/dump.cpp ../src/utils/code_block.cpp)
add_executable(test_dump_analyzer ${DUMP_ANALYZER_SOURCES} test_dump_analyzer.cpp)

set(ADDR_MANAGER_SOURCES ../src/utils/addr.cpp ../src/utils/code_block.cpp)
add_executable(test_addr_manager ${ADDR_MANAGER_SOURCES} test_addr_manager.cpp)

set(CODE_BLOCK_SOURCES ../src/utils/code_block.cpp)
add_executable(test_code_block ${CODE_BLOCK_SOURCES} test_code_block.cpp)

target_include_directories(test_dump_analyzer PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})
target_include_directories(test_addr_manager PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})
target_include_directories(test_code_block PRIVATE ${GTEST_INCLUDE_DIR} ${INCLUDE_DIRS})

target_link_libraries(test_dump_analyzer PRIVATE ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} pthread)
target_link_libraries(test_addr_manager PRIVATE ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} pthread)
target_link_libraries(test_code_block PRIVATE ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} pthread)