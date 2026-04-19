cmake_minimum_required( VERSION 3.20.0 )

project(
    wirewatch
    VERSION 1.0.0
    LANGUAGES C CXX
)

A113_Fnc_CloneRepo( "https://github.com/the-tcpdump-group/libpcap.git" "master" pcap "${CMAKE_CURRENT_LIST_DIR}/excom" )

add_executable( ${PROJECT_NAME} "main.cpp" )

target_link_libraries( ${PROJECT_NAME} 
    PUBLIC A113_OSp
    PUBLIC pcap
)