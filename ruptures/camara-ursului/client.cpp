#include "common.hpp"

struct Client {
    io::IPv4_TCP_socket   _server    = {};
    std::string           _token     = {};

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

    text::Fastcli   fastcli   = {
        {}, {
            {
                .text = "connect",
                .opts = {
                    { .sh0rt = 'a', .l0ng = "address", .arg = text::Fastcli::Arg_text, .fast_id = 0x0 },
                    { .sh0rt = 'p', .l0ng = "port", .arg = text::Fastcli::Arg_i32, .fast_id = 0x1 },
                    { .sh0rt = 't', .l0ng = "timeout-in", .arg = text::Fastcli::Arg_i32 },
                    { .sh0rt = 'T', .l0ng = "timeout-out", .arg = text::Fastcli::Arg_i32 },
                },
                .fnc = [ this ] ( auto& stencil_ ) -> auto {
                    const char*                   address = "127.0.0.1";
                    io::ipv4_port_t               port    = 58008;
                    io::IPv4_TCP_socket::config_t config  = {
                        .timeouts = { .outbound_s = CU_DEFAULT_CLIENT_OUTBOUND_TIMEOUT_S, .inbound_s = CU_DEFAULT_CLIENT_INBOUND_TIMEOUT_S }
                    };

                    char opt; while( opt = stencil_.next() ) {
                        switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                            case 'a': address = stencil_.arg_text().c_str(); break;
                            case 'p': port = (io::ipv4_port_t)stencil_.arg_i32(); break;
                            case 't': config.timeouts.inbound_s = stencil_.arg_i32(); break;
                            case 'T': config.timeouts.outbound_s = stencil_.arg_i32(); break;
                        }
                    }

                    CU_ASSERT_OR( A113_OK == _server.bind( address, port ) ) {
                        stencil_ += "Failed to bind to the give address and port.";
                        return A113_ERR_ENGINECALL;
                    }
                    CU_ASSERT_OR( A113_OK == _server.connect( config ) ) {
                        stencil_ += "Failed to connect to the given address.";
                        return A113_ERR_ENGINECALL;
                    }
                    return A113_OK;
                }
            }, {
                .text = "disconnect",
                .fnc = [ this ] ( auto& stencil_ ) -> auto {
                    _server.disconnect();
                    return A113_OK;
                }
            }, {
                .text = "register",
                .opts = {
                    { .sh0rt = 'n', .l0ng = "name", .arg = text::Fastcli::Arg_text, .fast_id = 0x0 }
                },
                .fnc = [ this ] ( auto& stencil_ ) -> auto {
                    nlohmann::json req{
                        { "verb", "register" }
                    };

                    char opt; while( opt = stencil_.next() ) {
                        switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                            case 'n': req[ "name" ] = stencil_.arg_text().c_str(); break;
                        }
                    }

                    nlohmann::json resp;
                    CU_ASSERT_OR( A113_OK == CU_request( _server, req.dump(), &resp ) ) {
                        stencil_ += "Request error.";
                        return A113_ERR_FLOW;
                    };
                    
                    auto token = resp.find( "token" );
                    CU_ASSERT_OR( token != resp.end() ) {
                        stencil_ += "Response packet with no token.";
                        return A113_ERR_FLOW;
                    }
                    _token = *token;

                    stencil_ += resp.dump(4);
                    return A113_OK;
                }
            }, {
                .text = "snoop",
                .opts = {
                    { .sh0rt = 't', .l0ng = "tag", .arg = text::Fastcli::Arg_text, .fast_id = 0x0 }
                },
                .fnc = [ this ] ( auto& stencil_ ) -> auto {
                    nlohmann::json req{
                        { "verb", "snoop" },
                        { "token", _token }
                    };

                    char opt; while( opt = stencil_.next() ) {
                        switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                            case 't': req[ "tag" ] = stencil_.arg_text().c_str(); break;
                        }
                    }

                    nlohmann::json resp;
                    CU_ASSERT_OR( A113_OK == CU_request( _server, req.dump(), &resp ) ) {
                        stencil_ += "Request error.";
                        return A113_ERR_FLOW;
                    };
                    
                    stencil_ += resp.dump(4);
                    return A113_OK;
                }
            }, {
                .text = "ur",
                .opts = {
                    { .sh0rt = 't', .l0ng = "text", .arg = text::Fastcli::Arg_text, .fast_id = 0x0 }
                },
                .fnc = [ this ] ( auto& stencil_ ) -> auto {
                    nlohmann::json req{
                        { "verb", "ur" },
                        { "token", _token }
                    };

                    char opt; while( opt = stencil_.next() ) {
                        switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                            case 't': req[ "text" ] = stencil_.arg_text().c_str(); break;
                        }
                    }

                    nlohmann::json resp;
                    CU_ASSERT_OR( A113_OK == CU_request( _server, req.dump(), &resp ) ) {
                        stencil_ += "Request error.";
                        return A113_ERR_FLOW;
                    };
                    
                    stencil_ += resp.dump(4);
                    return A113_OK;
                }
            }
        }
    };
};

#include <iostream>
int main( int argc, char* argv[] )  {
    init( argc, argv, { flags: InitFlags_Sockets } );

    Client client; 
    for(;;) {
        string cmd, out; getline( cin, cmd ); 
        CU_ASSERT_OR( A113_OK == client.fastcli( cmd, &out ) ) {
            spdlog::error( "{}", out );
        } else if( not out.empty() ) {
            spdlog::info( "{}", out );
        }
    }

    return 0x0;
}