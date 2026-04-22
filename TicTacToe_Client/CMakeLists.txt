cmake_minimum_required(VERSION 3.10)
project(TicTacToe)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    add_compile_options(-Wno-deprecated-declarations)
endif()

find_package(SFML 2.5 COMPONENTS graphics window system network audio REQUIRED)

set(RESOURCE_FILES "arial.ttf")

if(APPLE)
    set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    add_executable(TicTacToe MACOSX_BUNDLE tictactoe.cpp ${RESOURCE_FILES})
    target_link_libraries(TicTacToe sfml-graphics sfml-window sfml-system sfml-network sfml-audio)
else()
    find_package(SFML 2.5 COMPONENTS main REQUIRED)
    add_executable(TicTacToe WIN32 tictactoe.cpp ${RESOURCE_FILES})
    target_link_libraries(TicTacToe sfml-main sfml-graphics sfml-window sfml-system sfml-network sfml-audio)
endif()