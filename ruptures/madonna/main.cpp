#include "config.hpp"

#include <a113/gep/fastcli.hpp>
#include <a113/osp/madonna.hpp>

#include <iostream>

using namespace std; using namespace a113;

struct _bridge_t {
    std::atomic_bool   running   = { false };

} BridgE;

int main( int argc, char* argv[] ) {
    using namespace a113::text;
    Fastcli fastcli{
        {},
        { 
        {   .text = "exit",
            .fnc = [ & ] ( auto& stencil_ ) -> status_t {
                BridgE.running.store( false, std::memory_order_release );
                return A113_OK;
            }
        },{   .text = "proginfo",
            .opts = {
                { .sh0rt = 'v', .l0ng = "version" }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
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

    BridgE.running = true;
    for(; BridgE.running.load( std::memory_order_relaxed );) {
        std::string cmd, out;
        std::cout << std::format( "\n[{}]$ ", MDN_VERSION_STR );
        std::getline( std::cin, cmd );

        if( A113_OK == fastcli.execute( cmd, &out ) ) {
            mdn::BridgE->info( "\"{}\"\n{}", cmd, out );
        } else {
            mdn::BridgE->error( "\"{}\"\n{}", cmd, out );
        }
    }

}