#include <a113/osp/IO_sockets.hpp>
using namespace a113;

namespace crm {

};


int main( int argc, char* argv[] )  {
    init( argc, argv, { flags: InitFlags_Sockets } );

    io::IPv4_TCP_socket server;
    server.bind( "", 58008 );
    server.connect( {} );
    server.timeouts( {
        .outbound_ms = 10000,
        .inbound_ms  = 10000
    } );

    for(;;) {
        std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
    }

    return 0x0;
}