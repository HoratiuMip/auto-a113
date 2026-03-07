cmake_minimum_required( VERSION 3.28.0 )

project(
    rixory-experimental
    VERSION 1.0.0
    LANGUAGES C CXX
)

add_executable( ${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}/main.cpp" )
target_link_libraries( ${PROJECT_NAME} 
    PUBLIC A113_OSp 
    PUBLIC A113_ClkWrk_immersive
)
