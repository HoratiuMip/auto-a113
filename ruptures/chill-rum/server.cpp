#include <a113/osp/IO_sockets.hpp>
using namespace a113;

namespace crm {

};


int main( int argc, char* argv[] )  {
    init( argc, argv, { flags: InitFlags_Sockets } );

    io::IPv4_TCP_socket server;
    server.bind_peer( "0.0.0.0", 80 );
    server.listen();

    for(;;) {
        char buffer[ 256 ];
        size_t rcv_cnt = 0;

        spdlog::info( "Receiving..." );
        server.read( {
            .dst_ptr         = buffer,
            .dst_n           = sizeof( buffer ),
            .byte_count      = &rcv_cnt,
            .fail_if_not_all = false
        } );
        spdlog::info( "{}", std::string{ buffer, rcv_cnt } );

        std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    }

    return 0x0;
}