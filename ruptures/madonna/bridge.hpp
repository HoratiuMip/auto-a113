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

_A113_PROTECTED:
    status_t _north_init( void ) { return A113_OK; }
    status_t _south_init( void );

public:
    status_t init_all( void ) {
        this->_north_init();
        this->_south_init();
    }

public:
    status_t ui_frame( const clkwrk::Immersive::frame_cb_args_t& args_ ) {
        imm.assets_idle_splash_render();
        return A113_OK;
    }

}; inline _bridge_t BridgE;

}