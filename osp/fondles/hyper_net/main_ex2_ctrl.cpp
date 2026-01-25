#include <a113/osp/hyper_net.hpp>
#include <a113/OSp/IO_sockets.hpp>
#include <fstream>
#include <conio.h>

int main( int argc, char* argv[] ) {
    a113::init( argc, argv, a113::init_args_t{
        .flags = a113::InitFlags_Sockets
    } );

    a113::io::IPv4_TCP_socket client = {};

    spdlog::info( "Listening for client..." );
    client.bind_peer( a113::io::ipv4_addr_t{ 0x0 }, 9009 );
    A113_ASSERT_OR( 0x0 == client.listen() ) {
        return -0x1;
    }

    char red[]   = "R";
    char green[] = "G";

    for(;;) {
        switch( _getch() ) {
            case 'R': client.write( a113::io::port_W_desc_t{
                .src_ptr = red, .src_n = 1
            } ); spdlog::info( "Sent red." ); break;

            case 'G': client.write( a113::io::port_W_desc_t{
                .src_ptr = green, .src_n = 1
            } ); spdlog::info( "Sent green." ); break;
        }
    }

    return 0x0;
}