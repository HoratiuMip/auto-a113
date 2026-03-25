#include "bridge.hpp"

namespace mdn {

status_t _bridge_t::_south_init( void ) {
/* === command line interpreter === */
    using namespace a113::text;
    new (&cli) Fastcli{
        {},
        { 
        {   .text = "progctl",
            .opts = {
                { .sh0rt = 'c', .l0ng = "clear" },
                { .sh0rt = 'e', .l0ng = "exit" },
                { .sh0rt = 'v', .l0ng = "version" }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            std::system( "clear" );
                        break; }
                        case 'e': {
                            this->signal_shutdown();
                            return A113_OK;
                        }
                        case 'v': {
                            stencil_( "Madonna version: {}", MDN_VERSION_STR );
                        break; }
                    }
                }
                return A113_OK;
            }
        }, {
            .text = "roots",
            .opts = {
                { .sh0rt = 'c', .l0ng = "coeffs", .arg = Fastcli::Arg_f64, .fast_id = 0x0, .argc = Fastcli::Argc_multi_compact }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                std::vector< double > coeffs;

                char opt;  while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            coeffs = std::move( stencil_.arg_f64v() );
                        break; }
                    }
                }
        
                for( auto& root : mdn_1::roots( coeffs ) ) {
                    stencil_ += std::format( 
                        "{:+}{:+}i\n", root.real(), root.imag() 
                    );
                }
                return A113_OK;
            }
        }
        }
    };

    return A113_OK;
}

}