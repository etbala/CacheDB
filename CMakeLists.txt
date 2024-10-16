
cmake_minimum_required(VERSION 3.10)
project(CacheDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(server server.cpp)
add_executable(client client.cpp)

# enable_testing()
# add_executable(server_test server_test.cpp)
# target_link_libraries(server_test PRIVATE <dependencies-if-any>)
# add_test(NAME ServerTest COMMAND server_test)

# add_executable(client_test client_test.cpp)
# target_link_libraries(client_test PRIVATE <dependencies-if-any>)
# add_test(NAME ClientTest COMMAND client_test)