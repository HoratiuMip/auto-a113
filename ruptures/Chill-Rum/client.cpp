#include <a113/osp/IO_sockets.hpp>
#include "common.hpp"
using namespace std;
using namespace a113;

struct Client {
    io::IPv4_TCP_socket   _server    = {};

    atomic_bool           _running   = { false };
    thread                _rx_th     = {};

    int _rx_main( void ) {
        for(; _running.load( memory_order_relaxed ); ) {
            char buf[ 256 ];
            int  bc = 0;

            _server.read( {
                .dst_ptr    = buf,
                .dst_n      = sizeof( buf ),
                .byte_count = &bc
            } );

            if( bc > 0 ) {
                spdlog::info( "RX: \"{}\"", string{ buf, bc } );
            }
        }
    }

    string cli( const string& cmd_ ) {
    #define NEXT (*tok++)
        string ret   = std::format( "{} completed.", cmd_ );
        time_t t_now = time( nullptr );

        vector< string > toks;
        for( auto t : views::split( cmd_, ' ' ) ) toks.emplace_back( t.begin(), t.end() );
        auto tok = toks.begin();

        switch( hash_unsecure( NEXT ) ) {
            case hash_unsecure( "--connect" ): {
                _server.bind( NEXT.c_str(), DEFAULT_PORT );
                _server.connect( {} );

                _running.store( true, memory_order_release );
                _rx_th = thread( &Client::_rx_th, this );
            break; }

            case hash_unsecure( "--disconnect" ): {
                _server.disconnect();

                _running.store( false, memory_order_release );
                if( _rx_th.joinable() ) _rx_th.join();
            break; }

            case hash_unsecure( "--join-room" ): {
                int  buf_sz = 1 + tok->length() + 1;
                char buf[ buf_sz ]; strncpy( buf + 1, tok->c_str(), tok->length() );
                buf[ 1 + tok->length() ] = REQ_SPLT_CHR;

                _server.write( {
                    .src_ptr = buf,
                    .src_n   = buf_sz
                } );
            break; }
        }

        return ret;
    }
};

#include <iostream>
int main( int argc, char* argv[] )  {
    init( argc, argv, { flags: InitFlags_Sockets } );

    Client client; 
    for(;;) {
        string cmd; getline( cin, cmd ); 
        spdlog::info( "{}", client.cli( cmd ) );
    }

    return 0x0;
}