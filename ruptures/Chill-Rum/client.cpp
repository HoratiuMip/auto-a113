#include <a113/osp/IO_sockets.hpp>
#include "common.hpp"
using namespace std;
using namespace a113;

struct Client {
    io::IPv4_TCP_socket   server   = {};

    string cli( const string& cmd_ ) {
    #define NEXT (*tok++)
        string ret   = std::format( "{} completed.", cmd_ );
        time_t t_now = time( nullptr );

        vector< string > toks;
        for( auto t : views::split( cmd_, ' ' ) ) toks.emplace_back( t.begin(), t.end() );
        auto tok = toks.begin();

        switch( hash_unsecure( NEXT ) ) {
            case hash_unsecure( "--connect" ): {
                server.bind( NEXT.c_str(), DEFAULT_PORT );
                server.connect( {} );
            break; }

            case hash_unsecure( "--disconnect" ): {
                server.disconnect();
            break; }

            case hash_unsecure( "--join-room" ): {
                int  buf_sz = 1 + tok->length();
                char buf[ buf_sz ]; strncpy( buf + 1, tok->c_str(), tok->length() );

                server.write( {
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