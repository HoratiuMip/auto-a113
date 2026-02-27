cmake_minimum_required( VERSION 3.28.0 )

project(
    chill-rum
    VERSION 1.0.0
    LANGUAGES C CXX
)

set( client_exe "${PROJECT_NAME}-client" )
set( server_exe "${PROJECT_NAME}-server" )

add_executable( ${client_exe} "${CMAKE_CURRENT_LIST_DIR}/client.cpp" )
target_link_libraries( ${client_exe} PUBLIC A113_OSp )

add_executable( ${server_exe} "${CMAKE_CURRENT_LIST_DIR}/server.cpp" )
target_link_libraries( ${server_exe} PUBLIC A113_OSp )
