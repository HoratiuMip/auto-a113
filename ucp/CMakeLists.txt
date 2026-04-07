cmake_minimum_required( VERSION 3.20.0 )

file( GLOB _source_files "${CMAKE_CURRENT_LIST_DIR}/*.cpp" )

add_library( A113_uCp STATIC ${_source_files} )

target_link_libraries( A113_uCp PUBLIC A113_GEp )
target_include_directories( A113_uCp PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include" )

if( A113_Opt_TargetOS_FreeRTOS )
    target_compile_definitions( A113_uCp PUBLIC A113_TARGET_OS_FREERTOS )
endif()