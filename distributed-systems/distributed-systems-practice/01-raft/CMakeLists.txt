cmake_minimum_required(VERSION 3.10)
project(toy_raft CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -pthread)
add_link_options(-pthread)

include_directories(include)

add_library(raft_core src/raft_node.cpp)

add_executable(raft_node src/main.cpp)
target_link_libraries(raft_node raft_core)

add_executable(raft_client src/client.cpp)
target_link_libraries(raft_client raft_core)
