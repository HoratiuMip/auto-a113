#include <a113/osp/IO_sockets.hpp>
#include "common.hpp"
using namespace std;
using namespace a113;

struct Server {
// ======================= Fields =======================
    atomic_bool   _running   = { false };

    struct unsubscribed_t {
        io::IPv4_TCP_socket   client       = {};
        int                   failed_ops   = 0;
        time_t                born         = { time( nullptr ) };
    };
    list< unsubscribed_t >   _unsubscribed_list   = {};
    mutex                    _unsubscribed_mtx    = {};
    thread                   _unsubscribed_th     = {};

    struct chill_one_t {
        io::IPv4_TCP_socket   client  = {};
        string                name    = "_Unidentified"; 
    };
    struct room_t {
        string                name         = "";
        time_t                born         = { time( nullptr ) };
        list< chill_one_t >   chill_list   = {}; 
        mutex                 chill_mtx    = {}; 
    };  
    map< string, room_t >   _room_map   = {};
    mutex                   _room_mtx   = {};

// ======================= Utility =======================
    string _next_request_arg( const char** req_, int* len_ ) {
        const char* aux = strchr( *req_, REQ_SPLT_CHR );
        if( nullptr == aux ) return "";

        int    arg_len = aux - *req_;
        string ret{ *req_, arg_len };

        *req_ += arg_len + 1;
        *len_ -= arg_len + 1;

        return ret;
    }

    inline status_t _respond( io::IPv4_TCP_socket& client_, const string& resp_ ) {
        return client_.write( {
            .src_ptr = const_cast< string& >( resp_ ).data(),
            .src_n   = (int)resp_.length()
        } );
    }

// ======================= Mains =======================
    void _process_unsubscribed_request( const char* req_, int len_, decltype(_unsubscribed_list)::iterator* u_itr_  ) {
        string resp = "OK";

        switch( *req_++ ) {
            case OP_JOIN_ROOM: {
                string room_name = _next_request_arg( &req_, &len_ ); 
                if( room_name.empty() ) {
                    resp = "ILL-FORMED"; break;
                }
                
                room_t* room;
            {
                lock_guard lck{ _room_mtx };
                room = &_room_map[ room_name ];
            } {
                lock_guard lck{ room->chill_mtx };
                room->chill_list.emplace_back( move( (*u_itr_)->client ) );
                *u_itr_ = _unsubscribed_list.erase( *u_itr_ );
            }
            break; }
        }

        this->_respond( (*u_itr_)->client, resp );
    }

    void _unsubscribed_main( void ) {
        for(; _running.load( memory_order_relaxed );) {
            lock_guard lck{ _unsubscribed_mtx };

            time_t t_now = time( nullptr );
            for( auto u_itr = _unsubscribed_list.begin(); u_itr != _unsubscribed_list.end(); ) {
                unsubscribed_t& unsub = *u_itr;

                char buf[ MAX_PACKET_SIZE + 1 ] = { '\0' };
                int  bc                         = 0x0;
                int  rx_available               = -0x1;

                if( t_now - unsub.born >= DEFAULT_SERVER_UNSUBS_HOLD_TIME_S ) goto l_kill_unsub;

                if( A113_OK == unsub.client.holding_rx( &rx_available ) ) {
                    if( rx_available <= 0 ) goto l_itr_inc;
                } else goto l_unsub_op_fail;

                if( A113_OK == unsub.client.read( {
                    .dst_ptr    = buf,
                    .dst_n      = MAX_PACKET_SIZE,
                    .byte_count = &bc,
                    .req_all    = false,
                    .req_time   = true
                } ) && bc != 0x0 ) {
                    buf[ bc ] = '\0';
                    this->_process_unsubscribed_request( buf, bc, &u_itr );
                    goto l_no_itr_inc;
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
                .timeouts = { .outbound_s = DEFAULT_SERVER_OUTBOUND_TIMEOUT_S, .inbound_s = DEFAULT_SERVER_INBOUND_TIMEOUT_S }
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
    #define NEXT (*tok++)
        string ret   = "";
        time_t t_now = time( nullptr );

        vector< string > toks;
        for( auto t : views::split( cmd_, ' ' ) ) toks.emplace_back( t.begin(), t.end() );
        auto tok = toks.begin();

        switch( hash_unsecure( NEXT ) ) {
            case hash_unsecure( "--T-unsubs" ): {
                ret += "Unsubscribed clients list:\n";
                lock_guard lck{ _unsubscribed_mtx };

                if( _unsubscribed_list.empty() ) {
                    ret += "No unsubscribed clients.";
                } else {
                    int crt = 1;
                    for( unsubscribed_t& unsub : _unsubscribed_list ) {
                        ret += format( "{} {}:{} {}s\n", 
                            crt, 
                            unsub.client.addr_c_str(), unsub.client.port(), 
                            DEFAULT_SERVER_UNSUBS_HOLD_TIME_S - ( t_now - unsub.born )
                        );
                        ++crt;
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