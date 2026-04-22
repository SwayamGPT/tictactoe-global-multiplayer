cmake_minimum_required(VERSION 3.10)
project(TicTacToeServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# We only need the System and Network modules for a headless server!
find_package(SFML 2.5 COMPONENTS system network REQUIRED)

add_executable(Server server.cpp)
target_link_libraries(Server sfml-system sfml-network)
