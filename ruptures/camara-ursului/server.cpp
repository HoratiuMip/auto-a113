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
        string                     name           = {};
        thread                     main_th        = {};
        list< HVec< client_t > >   clients_list   = {};
        shared_mutex               clients_mtx    = {};
        time_t                     born           = { time( nullptr ) };
        time_t                     last_idle      = 0x0;
    };

// ======================= Fields =======================
    atomic_bool                       _running       = { false };

    atomic_int                        _track_id      = { 0x0 };
    map< int, HVec< client_t > >      _track_map     = {};
    shared_mutex                      _track_mtx     = {};

    map< string, HVec< pantry_t > >   _pantrys_map   = {};
    shared_mutex                      _pantrys_mtx   = {};

    list< HVec< client_t > >          _unsubs_list   = {};
    shared_mutex                      _unsubs_mtx    = {};
    thread                            _unsubs_th     = {};

// ======================= Utility =======================
    void _terminate_track( int track_id_ ) {
        lock_guard lck{ _track_mtx };

        auto itr = _track_map.find( track_id_ );
        CU_ASSERT_OR( itr != _track_map.end() ) return;

        itr->second->sock.disconnect();
        _track_map.erase( itr );
    }

    void _terminate_pantry( const string& name_ ) {
        lock_guard lck{ _pantrys_mtx };

        auto itr = _pantrys_map.find( name_ );
        CU_ASSERT_OR( itr != _pantrys_map.end() ) return;
        
        lock_guard lck2{ itr->second->clients_mtx };
        lock_guard lck3{ _unsubs_mtx };
        
        _unsubs_list.splice( _unsubs_list.end(), itr->second->clients_list );

        itr->second->last_idle = 0x1;
    }

    HVec< pantry_t > _create_or_get_pantry( const string& name_ ) {
        lock_guard lck{ _pantrys_mtx };

        auto& pan = _pantrys_map[ name_ ];
        if( pan ) return pan;
        
        pan = HVec< pantry_t >::make();
        pan->name = name_;

        thread( &Server::_pantry_main, this, pan ).detach();
        return pan;
    }

    status_t _create_pantry( string name_ ) { 
        lock_guard lck{ _pantrys_mtx };

        auto& pan = _pantrys_map[ name_ ];
        CU_ASSERT_OR( pan == nullptr ) return A113_ERR_WOULD_OVRWR;

        pan = HVec< pantry_t >::make();
        pan->name = move( name_ );
    
        thread( &Server::_pantry_main, this, pan ).detach();
        return A113_OK;
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

                if( t_now - unsub.last_xchg >= CU_DEFAULT_SERVER_UNSUBS_HOLD_TIME_S ) goto l_terminate_unsub;

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
                    CU_ASSERT_OR( verb != json.end() && verb->is_string() ) goto l_terminate_unsub;

                    switch( text::hash( *verb ) ) {
                        case text::hash( "register" ): {
                            unsub.name = move( json[ "name" ].get_ref< string& >() );
                            CU_ASSERT_OR( not unsub.name.empty() ) throw runtime_error{ "verb: register: empty name" };

                            unsub.token = HVec< token_t >::make();
                            unsub.token->populate( unsub );

                            CU_ASSERT_OR( A113_OK == CU_respond( unsub.sock, nlohmann::json{
                                { "msg", "You have successfully registered on the server." },
                                { "token", unsub.token->value }
                            }.dump() ) ) goto l_terminate_unsub;
                        break; }

                        case text::hash( "snoop" ): {
                            CU_ASSERT_OR( unsub.token ) throw runtime_error{ "verb: snoop: no token" };

                            string pan_name = move( json[ "name" ].get_ref< string& >() );
                            CU_ASSERT_OR( not pan_name.empty() ) throw runtime_error{ "verb: snoop: empty pantry name" };

                            auto pan = _create_or_get_pantry( pan_name );
                            CU_ASSERT_OR( A113_OK == CU_respond( unsub.sock, nlohmann::json{
                                { "msg", "Snooping pantry." },
                            }.dump() ) ) goto l_terminate_unsub;
                        {
                            lock_guard lck{ pan->clients_mtx };
                            pan->clients_list.emplace_back( move( *u_itr ) );
                        }
                            goto l_erase_unsub;
                        break; }
                    }
                } catch( const nlohmann::json::parse_error& err_ ) {
                    goto l_terminate_unsub;
                } catch( const runtime_error& err_ ) {
                    spdlog::warn( "{}:{} > {}", unsub.sock.addr_c_str(), unsub.sock.port(), err_.what() );
                    goto l_terminate_unsub;
                } catch( ... ) {
                    goto l_terminate_unsub;
                }
                goto l_itr_inc; 

            l_unsub_op_fail:
                if( not ++unsub.failed_ops >= SERVER_DROP_UNSUB_AFTER_FAIL_N ) goto l_itr_inc;
            l_terminate_unsub:
                _terminate_track( unsub.track );
            l_erase_unsub:
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

    void _pantry_main( HVec< pantry_t > pan_ ) {
        pantry_t& pan = *pan_;

        while( true ) {
            time_t t_now = time( nullptr );

            shared_lock lck{ pan.clients_mtx };
            if( pan.clients_list.empty() ) {
                if( pan.last_idle == 0x0 ) pan.last_idle = t_now;
                else if( t_now - pan.last_idle > CU_DEFAULT_SERVER_PANTRY_IDLE_ALLOW_TIME_S ) break;
            } else {
                pan.last_idle = 0x0;
            }

            this_thread::sleep_for( chrono::milliseconds( 100 ) );
        }
        
        lock_guard lck{ _pantrys_mtx };
        _pantrys_map.erase( pan.name );
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
                { .sh0rt = 't', .l0ng = "track", .arg = text::Fastcli::Arg_i32 },
                { .sh0rt = 'p', .l0ng = "pantry", .arg = text::Fastcli::Arg_text }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> auto {
                char opt; while( opt = stencil_.next() ) { 
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 't': _terminate_track( stencil_.arg_i32() ); break;
                        case 'p': _terminate_pantry( stencil_.arg_text() ); break; 
                    } 
                }
                return A113_OK;
            }
        }, {
            .text = "create-pantry",
            .opts = {
                { .sh0rt = 'n', .l0ng = "name", .arg = text::Fastcli::Arg_text }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> auto {
                const char* name = nullptr;

                char opt; while( opt = stencil_.next() ) { 
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'n': name = stencil_.arg_text().c_str(); break;
                    } 
                }

                CU_ASSERT_OR( name ) {
                    stencil_ += "Invalid pantry name."; return A113_ERR_BADARG;
                }

                _create_pantry( name );
                return A113_OK;
            }
        }, {
            .text = "list",
            .opts = {
                { .sh0rt = 'u', .l0ng = "unsubs" },
                { .sh0rt = 'p', .l0ng = "pantries" }
            },
            .fnc  = [ this ] ( auto& stencil_ ) -> auto {
                stencil_ += "\n";
                auto t_now = time( nullptr );

                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'u': {
                            shared_lock lck{ _unsubs_mtx };

                            stencil_ += "===== Unsubscribed clients =====\n";
                    
                            for( auto& unsub : _unsubs_list ) {
                                auto token = unsub->token;

                                stencil_ += format( "{}\tIp: {}:{}\tLease: {}s\tToken: {}\n",
                                    unsub->track, unsub->sock.addr_c_str(), unsub->sock.port(),
                                    CU_DEFAULT_SERVER_UNSUBS_HOLD_TIME_S - (t_now - unsub->last_xchg),
                                    token ? token->value : "NO_TOKEN"
                                );
                            }
                        break; }
                        case 'p': {
                            shared_lock lck{ _pantrys_mtx };

                            stencil_ += "===== Pantries =====\n";
                    
                            for( auto& [ name, pantry ] : _pantrys_map ) {
                                int snoop_count = pantry->clients_list.size();

                                stencil_ += format( "{}\tSnoopers: {}\n",
                                    name, snoop_count
                                );
                            }
                        break; }
                    }
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