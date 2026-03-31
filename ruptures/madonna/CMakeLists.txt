cmake_minimum_required( VERSION 3.28.0 )

project(
    madonna
    VERSION 1.0.0
    LANGUAGES C CXX
)

file( GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/docks/*.cpp" )

add_executable( ${PROJECT_NAME} 
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${srcs}"
)
target_link_libraries( ${PROJECT_NAME} 
    PUBLIC A113_OSp 
    PUBLIC A113_ClkWrk_immersive
)

target_include_directories( ${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/" )

