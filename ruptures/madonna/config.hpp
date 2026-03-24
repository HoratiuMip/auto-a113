#pragma once

#include <a113/gep/fastcli.hpp>
#include <a113/osp/madonna.hpp>

#include <a113/clkwrk/immersive.hpp>

#define MDN_VERSION_MAJOR 1
#define MDN_VERSION_MINOR 0
#define MDN_VERSION_PATCH 0
#define MDN_VERSION_STR "madonna-v1.0.0"

#define MDN_ASSERT_OR(c) A113_ASSERT_OR(c)

using namespace a113;

namespace mdn {

class _bridge_t {
public:
    _bridge_t( void ) {
        logger = spdlog::stdout_color_mt( "madonna" ); 
        logger->set_pattern( A113_SPDLOG_PATTERN );
        logger->info( "Bridge init OK." );
    }

_A113_PROTECTED:
    HVec< spdlog::logger >   logger   = nullptr;

public:
    A113_inline spdlog::logger* operator -> ( void ) { return logger.get(); }

public:
    std::atomic_bool    running   = { false };
    clkwrk::Immersive   imm       = {};
    text::Fastcli       cli       = {};

public:
    void signal_shutdown( void ) {
        running.store( false, std::memory_order_release );
    }

public:
    status_t init( void ) {
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

_A113_PROTECTED:

public:
    status_t ui_frame( const clkwrk::Immersive::frame_cb_args_t& args_ ) {
        imm.assets_idle_splash_render();
        return A113_OK;
    }

}; inline _bridge_t BridgE;

}