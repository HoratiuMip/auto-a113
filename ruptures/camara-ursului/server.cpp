#include "common.hpp"

struct Server {
// ======================= Consts =======================

// ======================= Structures =======================
    struct client_t;
    struct room_t;

    struct token_t {
        string   value   = {};

        status_t populate( const client_t& client_ ); 
    };

    struct client_t {
        int                   track        = 0x0;
        io::IPv4_TCP_socket   sock         = {};
        HVec< token_t >       token        = nullptr;
        string                name         = {};
        HVec< room_t >        room         = nullptr;
        int                   failed_ops   = 0;
        time_t                born         = { time( nullptr ) };
        time_t                last_xchg    = { time( nullptr ) };
    };

    struct pantry_t {
        int                        track          = 0x0;
        list< HVec< client_t > >   clients_list   = {};
        shared_mutex               clients_mtx    = {};
        time_t                     born           = { time( nullptr ) };
    };

// ======================= Fields =======================
    atomic_bool                     _running       = { false };

    atomic_int                      _track_id      = { 0x0 };
    map< int, HVec< client_t > >    _track_map     = {};
    shared_mutex                    _track_mtx     = {};

    list< HVec< client_t > >        _unsubs_list   = {};
    shared_mutex                    _unsubs_mtx    = {};
    thread                          _unsubs_th     = {};

// ======================= Utility =======================
    void _terminate_by_track_id( int track_id ) {
        lock_guard lck{ _track_mtx };

        auto itr = _track_map.find( track_id );
        CU_ASSERT_OR( itr != _track_map.end() ) return;

        itr->second->sock.disconnect();
        _track_map.erase( itr );
    }

// ======================= Mains =======================
    void _unsubs_main( void ) {
        for(; _running.load( memory_order_relaxed );) {
            lock_guard lck{ _unsubs_mtx };

            for( auto u_itr = _unsubs_list.begin(); u_itr != _unsubs_list.end(); ) {
                time_t    t_now = time( nullptr );
                client_t& unsub = **u_itr;

                char buffer[ CU_MAX_PACKET_SIZE ];
                int  byte_count      = 0x0;
                int  bytes_available = -0x1;

                if( t_now - unsub.last_xchg >= CU_DEFAULT_SERVER_UNSUBS_HOLD_TIME_S ) goto l_kill_unsub;

                if( A113_OK == unsub.sock.holding_rx( &bytes_available ) ) {
                    if( bytes_available <= 0 ) goto l_itr_inc;
                } else goto l_unsub_op_fail;

                CU_ASSERT_OR( A113_OK == unsub.sock.read( {
                    .dst_ptr    = buffer,
                    .dst_n      = CU_MAX_PACKET_SIZE,
                    .byte_count = &byte_count,
                    .req_all    = false,
                    .req_time   = true
                } ) && byte_count > 0x0 ) goto l_unsub_op_fail;
                
                unsub.last_xchg = time( nullptr );
                try {
                    auto json = nlohmann::json::parse( buffer, buffer + byte_count );
                    auto verb = json.find( "verb" );
                    CU_ASSERT_OR( verb != json.end() && verb->is_string() ) goto l_kill_unsub;

                    switch( text::hash( *verb ) ) {
                        case text::hash( "register" ): {
                            unsub.name = move( json[ "name" ].get_ref< string& >() );
                            CU_ASSERT_OR( not unsub.name.empty() ) throw runtime_error{ "v: register: empty name" };

                            unsub.token = HVec< token_t >::make();
                            unsub.token->populate( unsub );

                            CU_ASSERT_OR( A113_OK == CU_respond( unsub.sock, nlohmann::json{
                                { "status_str", "OK" },
                                { "msg", "You have successfully registered on the server." },
                                { "token", unsub.token->value }
                            }.dump() ) ) goto l_kill_unsub;
                        break; }
                    }
                } catch( const nlohmann::json::parse_error& err_ ) {
                    goto l_kill_unsub;
                } catch( const runtime_error& err_ ) {
                    spdlog::warn( "{}:{} > {}", unsub.sock.addr_c_str(), unsub.sock.port(), err_.what() );
                    goto l_kill_unsub;
                } catch( ... ) {
                    goto l_kill_unsub;
                }
                goto l_itr_inc; 

            l_unsub_op_fail:
                if( not ++unsub.failed_ops >= SERVER_DROP_UNSUB_AFTER_FAIL_N ) goto l_itr_inc;
            l_kill_unsub:
                u_itr = _unsubs_list.erase( u_itr );
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
        
        _unsubs_th = thread( &Server::_unsubs_main, this );

        io::IPv4_TCP_socket server; server.bind( "0.0.0.0", DEFAULT_PORT ); 
        server.listen();

        for(; _running.load( memory_order_relaxed );) {
            io::IPv4_TCP_socket client_sock;
            if( A113_OK == server.accept( &client_sock, {
                .timeouts = { .outbound_s = DEFAULT_SERVER_OUTBOUND_TIMEOUT_S, .inbound_s = DEFAULT_SERVER_INBOUND_TIMEOUT_S }
            } ) ) {
                auto client = HVec< client_t >::make();

                new (&client->sock) io::IPv4_TCP_socket{ move( client_sock ) };
                client->track = _track_id.fetch_add( 0x1, std::memory_order_relaxed );
            {
                lock_guard lck{ _track_mtx };
                _track_map[ client->track ] = client;
            } 
            {
                lock_guard lck{ _unsubs_mtx };
                _unsubs_list.emplace_back( move( client ) );
            }
            } else {
                this_thread::sleep_for( chrono::milliseconds( 100 ) );
            }
        }

        return 0x0;
    }

// ======================= Command line =======================
    text::Fastcli   fastcli   = { 
    {}, {
        {
            .text = "terminate",
            .opts = {
                { .sh0rt = 't', .l0ng = "track-id", .arg = text::Fastcli::Arg_i32 }
            },
            .fnc  = [ this ] ( auto& stencil_ ) -> auto {
                optional< int > track_id = {};

                char opt; while( opt = stencil_.next() ) { 
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 't': track_id = stencil_.arg_i32(); break;
                    } 
                }

                if( track_id.has_value() ) {
                    _terminate_by_track_id( track_id.value() );
                }

                return A113_OK;
            }
        }, {
            .text = "list-unsubs",
            .opts = {},
            .fnc  = [ this ] ( auto& stencil_ ) -> auto {
                shared_lock lck{ _unsubs_mtx };

                stencil_ += "\n";
                auto t_now = time( nullptr );

                for( auto& unsub : _unsubs_list ) {
                    auto token = unsub->token;

                    stencil_ += format( "{}\tIp: {}:{}\tLease: {}s\tToken: {}\n",
                        unsub->track, unsub->sock.addr_c_str(), unsub->sock.port(),
                        CU_DEFAULT_SERVER_UNSUBS_HOLD_TIME_S - (t_now - unsub->last_xchg),
                        token ? token->value : "NO_TOKEN"
                    );
                }
                return A113_OK;
            }
        }
    } };

};

status_t Server::token_t::populate( const Server::client_t& client_ ) {
    value = format( "{}:{}/{}/{}", 
        client_.sock.addr_c_str(), client_.sock.port(), time( nullptr ), client_.name
    );
    return A113_OK;
}

#include <iostream>
int main( int argc, char* argv[] )  {
    init( argc, argv, { .flags = InitFlags_Sockets } );

    Server server; auto server_th = jthread( &Server::main, &server, argc, argv );
    for(;;) {
        string cmd, out; getline( cin, cmd ); 
        CU_ASSERT_OR( A113_OK == server.fastcli( cmd, &out ) ) {
            spdlog::error( "{}", out );
        } else if( not out.empty() ) {
            spdlog::info( "{}", out );
        }
    }

    return 0x0;
}