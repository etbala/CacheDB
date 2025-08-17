cmake_minimum_required(VERSION 3.20)
project(CacheDB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # For Windows: Prevent overriding the parent project's compiler/linker settings
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE CLIENT_FILES "client/*.cc" "client/*.cpp")
file(GLOB_RECURSE SERVER_FILES "server/*.cc" "server/*.cpp")
file(GLOB_RECURSE TEST_FILES "tests/*.cc" "tests/*.cpp")

add_executable(cachedb_client ${CLIENT_FILES} ${CMAKE_SOURCE_DIR}/client.cpp)
target_include_directories(cachedb_client
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/client
)

add_executable(cachedb_server ${SERVER_FILES} ${CMAKE_SOURCE_DIR}/cachedb.cpp)
target_include_directories(cachedb_server
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/server
)

add_executable(all_tests ${CLIENT_FILES} ${SERVER_FILES} ${TEST_FILES})
target_link_libraries(all_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(all_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/server
)

include(GoogleTest)
gtest_discover_tests(all_tests)
