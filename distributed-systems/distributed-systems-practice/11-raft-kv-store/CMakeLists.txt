cmake_minimum_required(VERSION 3.10)
project(raft_kv_store CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options(-Wall -Wextra -pthread)
add_link_options(-pthread)

include_directories(include)

add_library(raft_core src/raft_node.cpp)

# A second build of the exact same raft_node.cpp, compiled with
# RAFT_UNSAFE_READS so the partition demo (scripts/partition_demo.sh) can
# run the identical scenario against Project 1's original, NOT-fully-
# linearizable GET check and show the actual difference — see
# ANALYSIS.md and the #ifdef in raft_node.cpp's handleClientRequestMsg.
# History (ANALYSIS.md bug #6): raft_node.h used to add a member under this
# macro, which changes sizeof(RaftNode). With the definition PRIVATE, only
# raft_node.cpp saw the larger layout while main.cpp (compiled separately
# into raft_node_unsafe) saw the smaller one, and main()'s inline
# isRunning() read running_ at the wrong offset — the binary exited ~1ms
# after starting. The root cause is now removed: no header-visible member
# or layout depends on this macro any more (it only selects code inside
# raft_node.cpp). PUBLIC is kept anyway as defense in depth, so that any
# future header-visible use of the macro stays consistent across every
# translation unit that links this library.
add_library(raft_core_unsafe src/raft_node.cpp)
target_compile_definitions(raft_core_unsafe PUBLIC RAFT_UNSAFE_READS)

add_library(kv_client src/kv_client.cpp)
target_link_libraries(kv_client raft_core)

add_executable(raft_node src/main.cpp)
target_link_libraries(raft_node raft_core)

add_executable(raft_node_unsafe src/main.cpp)
target_link_libraries(raft_node_unsafe raft_core_unsafe)

add_executable(kvctl src/kvctl.cpp)
target_link_libraries(kvctl kv_client raft_core)

add_executable(direct_probe src/direct_probe.cpp)
target_link_libraries(direct_probe raft_core)
