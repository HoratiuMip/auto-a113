cmake_minimum_required( VERSION 3.28.0 )

set( _excom_dir "${CMAKE_CURRENT_LIST_DIR}/excom" )

configure_file( "${CMAKE_CURRENT_LIST_DIR}/include/A113/brp/descriptor.hpp.in" "${CMAKE_CURRENT_LIST_DIR}/include/A113/brp/descriptor.hpp" )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" )

add_library( A113_BRp STATIC ${_source_files} )

target_include_directories( A113_BRp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )
