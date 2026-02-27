#include <a113/osp/IO_sockets.hpp>
using namespace a113;

namespace crm {

};


int main( int argc, char* argv[] )  {
    init( argc, argv, { flags: InitFlags_Sockets } );

    io::IPv4_TCP_socket server;
    server.bind_peer( "127.0.0.1", 80 );
    server.uplink();

    for(;;) {
        char msg[] = "Hello there!";

        spdlog::info( "Sending..." );
        server.write( {
            .src_ptr         = msg,
            .src_n           = sizeof( msg ),
            .byte_count      = nullptr,
            .fail_if_not_all = false
        } );

        std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    }

    return 0x0;
}