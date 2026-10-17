cmake_minimum_required(VERSION 3.10)
project(sharded_kv CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -pthread)
add_link_options(-pthread)

include_directories(include)

add_library(raft_core src/raft_node.cpp)

add_library(kv_client src/kv_client.cpp)
target_link_libraries(kv_client raft_core)

add_executable(raft_node src/main.cpp)
target_link_libraries(raft_node raft_core)

add_executable(kvctl src/kvctl.cpp)
target_link_libraries(kvctl kv_client raft_core)

add_executable(direct_probe src/direct_probe.cpp)
target_link_libraries(direct_probe raft_core)

add_executable(ring_stats src/ring_stats.cpp)

add_library(sharded_client src/sharded_client.cpp)
target_link_libraries(sharded_client kv_client raft_core)

add_executable(shardctl src/shardctl.cpp)
target_link_libraries(shardctl sharded_client)

add_executable(shardtool src/shardtool.cpp)
target_link_libraries(shardtool sharded_client)
add_executable(hash_ablation src/hash_ablation.cpp)
