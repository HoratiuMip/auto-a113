#pragma once

#include <a113/gep/fastcli.hpp>
#include <a113/osp/madonna.hpp>

#include <a113/clkwrk/immersive.hpp>

#define MDN_VERSION_MAJOR 1
#define MDN_VERSION_MINOR 0
#define MDN_VERSION_PATCH 0
#define MDN_VERSION_STR "madonna-v1.0.0"

#define MDN_ASSERT_OR(c) A113_ASSERT_OR(c)

#define MDN_IN
#define MDN_OUT
#define MDN_IN_OPT
#define MDN_OUT_OPT
#define MDN_IN_OUT
#define MDN_IN_OUT_OPT

namespace mdn {

#define MDN_DOCK_NAME_FNC \
    virtual std::string_view name( void )

#define MDN_DOCK_GUIX_FNC \
    virtual a113::status_t guix_frame( \
        MDN_IN   const a113::clkwrk::Immersive::frame_cb_args_t&   args_ \
    )

#define MDN_DOCK_FAST_AUTO_INSTALL( t, ... ) \
    static struct _mdn_auto_install_t_ { \
        _mdn_auto_install_t_( void ) { \
            BridgE.install( a113::HVec< t >::make( __VA_ARGS__ ) ); \
        } \
    } _mdn_auto_install_; 

class dock_t {
public:
    friend class _bridge_t;

protected:
    std::string   _id   = {};

protected:
    MDN_DOCK_NAME_FNC { return "unknown"; }
    MDN_DOCK_GUIX_FNC { return A113_OK; }

};

class _bridge_t : public a113::bridge_t {
public:
    _bridge_t( void ) : bridge_t{ MDN_VERSION_STR } {
        logger->info( "bridge: init done." );
    }

// ======================= Fields =======================
public:
    a113::clkwrk::Immersive                              imm          = {};

protected:
    std::atomic< a113::status_t >                        _status      = { A113_ERR_TERMINATED };

    std::map< std::string_view, a113::HVec< dock_t > >   _docks       = {};
    std::shared_mutex                                    _docks_mtx   = {};

    std::thread                                          _th_guix     = {};

public:
    A113_inline auto status( void ) { return _status.load( std::memory_order_relaxed ); }

    A113_inline void wait_stop( void ) { _status.wait( A113_OK, std::memory_order_seq_cst ); this->stop(); }

public:
    a113::status_t install( 
        MDN_IN   a113::HVec< dock_t >    dock_ 
    ) {
        MDN_ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null hvec." );
            return A113_ERR_BADARG;
        }

        dock_->_id = std::format( "{}-{:x}", dock_->name(), time( nullptr ) );

        std::string_view sv = dock_->_id;
        MDN_ASSERT_OR( not sv.empty() ) {
            logger->error( "bridge: install dock: empty id." ); 
            return A113_ERR_FLOW;
        }

        std::unique_lock lck{ _docks_mtx };
        auto& dock = _docks[ sv ];
        MDN_ASSERT_OR( not dock ) {
            lck.unlock();
            logger->error( "bridge: install dock: \"{}\" already exists.", sv ); 
            return A113_ERR_WOULD_OVRWR;
        }

        dock = std::move( dock_ );
        lck.unlock();

        logger->info( "bridge: installed dock \"{}\".", sv );
        return A113_OK;
    }

    void uninstall( 
        MDN_IN   std::string_view   dock_id_
    ) {
        std::unique_lock lck{ _docks_mtx };
        auto dock = _docks.extract( dock_id_ );
        lck.unlock();

        logger->info( "bridge: uninstalled dock \"{}\".", dock.mapped()->_id );
    }

public:
    a113::status_t start( void ) {
        MDN_ASSERT_OR( not _th_guix.joinable() && this->status() != A113_OK ) {
            logger->error( "bridge: start: already running." );
            return A113_ERR_LOGIC;
        }

        _status.store( A113_OK, std::memory_order_release );

        _th_guix = std::thread( &a113::clkwrk::Immersive::main, &imm, 0, nullptr, a113::clkwrk::Immersive::config_t{
            .ctx        = nullptr,
            .title      = MDN_VERSION_STR,
            .width      = 680,
            .height     = 680,
            .srf_bgn_as = a113::clkwrk::Immersive::SrfBeginAs_Default,
            .init_cb    = [ this ] ( const auto& args_ ) -> auto {
                ImGui::StyleColorsClassic();     
                imm.imgui.io->FontGlobalScale = 1.22f;
                imm->disengage_face_culling();   
                return A113_OK;
            },
            .loop_cb    = [ this ] ( const auto& args_ ) -> auto { 
                return this->guix_frame( args_ );
            },
            .exit_cb    = [ this ] ( const auto& args_ ) -> auto { 
                this->signal_stop();
                return A113_OK;
            }
        } );

        logger->info( "bridge: started." );
        return A113_OK;
    }

    void signal_stop( void ) {
        _status.store( A113_ERR_TERMINATED, std::memory_order_release );
        _status.notify_all();
        logger->info( "bridge: stopping..." );
    }

    void stop( void ) {
        this->signal_stop();
        if( _th_guix.joinable() ) _th_guix.join();
        logger->info( "bridge: stopped." );
    }

public:
    MDN_DOCK_GUIX_FNC {
        imm.assets_idle_splash_render( args_ );

        std::shared_lock lck{ _docks_mtx };
        for( auto& [ id, dock ] : _docks ) {
            dock->guix_frame( args_ );
        }

        return A113_OK;
    }

}; inline _bridge_t BridgE;

}