#pragma once

#include <a113/gep/fastcli.hpp>
#include <a113/osp/dispenser.hpp>
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

class proxy_t {
public:
    friend class _bridge_t;

public:
    proxy_t( 
        MDN_IN   const std::string&                      name_, 
        MDN_IN   const a113::text::Fastcli::config_t&    cli_config_, 
        MDN_IN   const a113::text::Fastcli::cmd_map_t&   cli_cmd_map_
    ) : _name{ name_ }, _cli{ cli_config_, cli_cmd_map_ } {}

protected:
    std::string           _name   = {};
    a113::text::Fastcli   _cli    = {};

public:
    a113::status_t pass( const std::string& text_, std::string* out_ ) {
        return _cli.execute( text_, out_ );
    }

};

#define MDN_DOCK_NAME_FNC \
    virtual std::string_view name( void )

#define MDN_DOCK_GUIX_FNC \
    virtual a113::status_t guix_frame( \
        MDN_IN   const a113::clkwrk::Immersive::frame_cb_args_t&   args_ \
    )

class dock_t {
public:
    friend class _bridge_t;

protected:
    std::string   _id   = {};

protected:
    MDN_DOCK_NAME_FNC { return "unknown"; }
    MDN_DOCK_GUIX_FNC { return A113_OK; }

};

#define MDN_AUTO_INSTALL( t, ... ) \
    static struct _mdn_installer_##t##_t_ { \
        _mdn_installer_##t##_t_( void ) { \
            BridgE.install( a113::HVec< t >::make( __VA_ARGS__ ) ); \
        } \
    } _mdn_installer_##t##_; 

class _bridge_t : public a113::bridge_t {
public:
    _bridge_t( void ) : bridge_t{ MDN_VERSION_STR } {
        logger->info( "bridge: init done." );
    }

// ======================= Fields =======================
public:
    a113::clkwrk::Immersive                               imm           = {};

protected:
    std::atomic< a113::status_t >                         _status       = { A113_ERR_TERMINATED };

    std::map< std::string_view, a113::HVec< proxy_t > >   _proxys       = {};
    std::recursive_mutex                                  _proxys_mtx   = {};

    std::map< std::string_view, a113::HVec< dock_t > >    _docks        = {};
    std::recursive_mutex                                  _docks_mtx    = {};

    std::thread                                           _th_guix      = {};

public:
    A113_inline auto status( void ) { return _status.load( std::memory_order_relaxed ); }

    A113_inline void wait_stop( void ) { _status.wait( A113_OK, std::memory_order_seq_cst ); this->stop(); }

public:
    a113::status_t install(
        MDN_IN   a113::HVec< proxy_t>   proxy_
    ) {
        MDN_ASSERT_OR( proxy_ ) {
            logger->error( "bridge: install proxy: null proxy." );
            return A113_ERR_BADARG;
        }

        std::string_view sv = proxy_->_name;
        MDN_ASSERT_OR( not sv.empty() ) {
            logger->error( "bridge: install proxy: empty name." ); 
            return A113_ERR_FLOW;
        }

        std::unique_lock lck{ _proxys_mtx };
        auto& proxy = _proxys[ sv ];
        MDN_ASSERT_OR( not proxy ) {
            lck.unlock();
            logger->error( "bridge: register proxy: \"{}\" already exists.", sv );
            return A113_ERR_WOULD_OVRWR;
        }

        proxy = std::move( proxy_ );
        lck.unlock();

        logger->info( "bridge: installed proxy \"{}\".", sv );
        return A113_OK;
    }

    void uninstall_proxy(
        MDN_IN   const std::string&   proxy_name_  
    ) {
        std::unique_lock lck{ _proxys_mtx };
        _proxys.erase( proxy_name_ );
        lck.unlock();

        logger->info( "bridge: uninstalled proxy \"{}\".", proxy_name_ );
    }

    a113::status_t install( 
        MDN_IN   a113::HVec< dock_t >    dock_ 
    ) {
        MDN_ASSERT_OR( dock_ ) {
            logger->error( "bridge: install dock: null dock." );
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

    void uninstall_dock( 
        MDN_IN   const std::string&   dock_id_
    ) {
        std::unique_lock lck{ _docks_mtx };
        _docks.erase( dock_id_ );
        lck.unlock();

        logger->info( "bridge: uninstalled dock \"{}\".", dock_id_ );
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
    a113::status_t proxy_pass( 
        MDN_IN    std::string_view     proxy_name_, 
        MDN_IN    const std::string&   command_, 
        MDN_OUT   std::string*         out_
    ) {
        std::unique_lock lck{ _proxys_mtx };
        auto itr = _proxys.find( proxy_name_ );

        MDN_ASSERT_OR( itr != _proxys.end() ) {
            lck.unlock();
            *out_ += std::format( "No such proxy: \"{}\".", proxy_name_ );
            return A113_ERR_BADARG;
        }

        auto proxy = itr->second; lck.unlock();

        return proxy->pass( command_, out_ );
    }

public:
    MDN_DOCK_GUIX_FNC {
        imm.assets_idle_splash_render( args_ );

        std::unique_lock lck{ _docks_mtx };
        std::erase_if( _docks, [ &args_ ] ( auto& itr_ ) -> bool {
            return A113_OK != itr_.second->guix_frame( args_ );
        } );

        return A113_OK;
    }

}; inline _bridge_t BridgE;

}