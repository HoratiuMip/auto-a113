cmake_minimum_required( VERSION 3.28.0 )

set( _excom_dir "${CMAKE_CURRENT_LIST_DIR}/excom" )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" )

add_library( A113_GEp STATIC ${_source_files} )

target_include_directories( A113_GEp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )
