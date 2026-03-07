#include "common.hpp"

#include <a113/osp/IO_sockets.hpp>
using namespace std;
using namespace a113;

struct Server {
// ======================= Fields =======================
    atomic_bool   _running   = { false };

    struct unsubscribed_t {
        io::IPv4_TCP_socket   client       = {};
        int                   failed_ops   = 0;
    };
    list< unsubscribed_t >   _unsubscribed_list   = {};
    mutex                    _unsubscribed_mtx    = {};
    thread                   _unsubscribed_th     = {};

    struct chill_one_t {

    };
    struct room_t {
        string                        name          = "";
        list< io::IPv4_TCP_socket >   client_list   = {}; 
        mutex                         client_mtx    = {}; 
    };  
    map< string, room_t >   _room_list   = {};
    mutex                   _room_mtx    = {};

// ======================= Utility =======================
    static constexpr uint32_t hash( const string& str_ ) {
        uint32_t h = 2166136261U;
        for( char c : str_ ) {
            h ^= (uint32_t)c;
            h *= 16777619U;
        }
        return h;
    }

// ======================= Mains =======================
    void _unsubscribed_main( void ) {
        for(; _running.load( memory_order_relaxed );) {
            lock_guard lck{ _unsubscribed_mtx };

            for( auto u_itr = _unsubscribed_list.begin(); u_itr != _unsubscribed_list.end(); ) {
                unsubscribed_t& unsub = *u_itr;

                char buf[ MAX_PACKET_SIZE ] = { '\0' };
                int  bc                     = 0x0;
                int  rx_available           = -0x1;

                if( A113_OK == unsub.client.holding_rx( &rx_available ) ) {
                    if( rx_available <= 0 ) goto l_itr_inc;
                } else goto l_unsub_op_fail;

                if( A113_OK == unsub.client.read( {
                    .dst_ptr    = buf,
                    .dst_n      = sizeof( buf ),
                    .byte_count = &bc,
                    .req_all    = false,
                    .req_time   = true
                } ) && bc != 0x0 ) {

                } else goto l_unsub_op_fail;

                goto l_itr_inc; 

            l_unsub_op_fail:
                if( not ++unsub.failed_ops >= SERVER_DROP_UNSUB_AFTER_FAIL_N ) goto l_itr_inc;
            l_kill_unsub:
                u_itr = _unsubscribed_list.erase( u_itr );
                goto l_no_itr_inc;
            l_itr_inc:
                ++u_itr;
            l_no_itr_inc:
                continue;
            }
        }
        this_thread::sleep_for( chrono::milliseconds{ 100 } );
    }

    int main( int argc_, char* argv_[] ) {
        _running.store( true, memory_order_release );
        
        _unsubscribed_th = thread( &Server::_unsubscribed_main, this );

        io::IPv4_TCP_socket server; server.bind( "0.0.0.0", DEFAULT_PORT ); 
        server.listen();

        for(; _running.load( memory_order_relaxed );) {
            io::IPv4_TCP_socket client;
            if( A113_OK == server.accept( &client, {
                .timeouts = { .outbound_ms = DEFAULT_SERVER_OUTBOUND_TIMEOUT_MS, .inbound_ms = DEFAULT_SERVER_INBOUND_TIMEOUT_MS }
            } ) ) {
                lock_guard lck{ _unsubscribed_mtx };
                _unsubscribed_list.emplace_back( unsubscribed_t{
                    .client = move( client )
                } );
            } else {

            }
        }

        return 0x0;
    }

    string cli( const string& cmd_ ) {
        string ret = "";

        switch( hash( cmd_ ) ) {
            case hash( "--T-unsubs" ): {
                ret += "Unsubscribed clients list:\n";
                lock_guard lck{ _unsubscribed_mtx };

                if( _unsubscribed_list.empty() ) {
                    ret += "No unsubscribed clients.";
                } else {
                    int crt = 1;
                    for( unsubscribed_t& unsub : _unsubscribed_list ) {
                        ret += format( "{} {}\n", crt, unsub.client.addr_c_str() );
                    }
                }
            break; }
        }

        return ret;
    }
};

#include <iostream>
int main( int argc, char* argv[] )  {
    init( argc, argv, { .flags = InitFlags_Sockets } );

    Server server; auto server_th = jthread( &Server::main, &server, argc, argv );
    for(;;) {
        string cmd; getline( cin, cmd ); 
        spdlog::info( "{}", server.cli( cmd ) );
    }

    return 0x0;
}