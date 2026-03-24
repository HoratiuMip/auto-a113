#include "config.hpp"

#include <iostream>

int main( int argc, char* argv[] ) {
    using namespace mdn;

    init( argc, argv, init_args_t{
        .flags = InitFlags_None
    } );

    BridgE.init();

    BridgE.imm.main( argc, argv, clkwrk::Immersive::config_t{
        .ctx        = nullptr,
        .title      = MDN_VERSION_STR,
        .width      = 680,
        .height     = 680,
        .srf_bgn_as = clkwrk::Immersive::SrfBeginAs_Default,
        .init_cb    = [] ( const clkwrk::Immersive::init_cb_args_t& args_ ) static -> status_t {
            ImGui::StyleColorsClassic();     
            BridgE.imm.cluster().disengage_face_culling();   
            return A113_OK;
        },
        .loop_cb    = [] ( const clkwrk::Immersive::frame_cb_args_t& args_ ) static -> status_t { 
            return BridgE.ui_frame( args_ );
        },
        .exit_cb    = [] ( const clkwrk::Immersive::exit_cb_args_t& args_ ) static -> status_t { 
            return A113_OK;
        }
    } );

    std::this_thread::sleep_for( std::chrono::seconds{ 2 } );

    BridgE.running = true;
    for(; BridgE.running.load( std::memory_order_relaxed );) {
        std::string cmd, out;
        std::cout << std::format( "\n[{}]$ ", MDN_VERSION_STR );
        std::getline( std::cin, cmd );

        if( A113_OK == BridgE.cli.execute( cmd, &out ) ) {
            BridgE->info( "\"{}\"\n{}", cmd, out );
        } else {
            BridgE->error( "\"{}\"\n{}", cmd, out );
        }
    }
}