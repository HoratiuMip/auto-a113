cmake_minimum_required( VERSION 3.30.0 )

add_executable( "a113_fndl-hyper_net" main.cpp )
target_link_libraries( "a113_fndl-hyper_net"
    PUBLIC A113_OSp
)

add_executable( "a113_fndl-hyper_net_ex2_ctrl" main_ex2_ctrl.cpp )
target_link_libraries( "a113_fndl-hyper_net_ex2_ctrl"
    PUBLIC A113_OSp
)