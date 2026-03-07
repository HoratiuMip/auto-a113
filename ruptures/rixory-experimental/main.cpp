#include <a113/clkwrk/immersive.hpp>
#include <imgui.h>
#include <implot.h>

#include <a113/osp/rixrat_core.hpp>

using namespace std; using namespace a113;

clkwrk::Immersive G_imm;

status_t ui_frame( const clkwrk::Immersive::frame_cb_args_t& args_ ) {
    static constexpr int NX = 200, NY = 300;
    static vector< float > grid_x( NX );
    static vector< float > grid_y( NY );
    static vector< float > grid_f( NX * NY );
    static vector< float > grid_g( NX * NY );
    static float MSE = 0.0;

    static GLuint fbo;
    static HVec< imm::tex_t > tex;

    static auto _do_once_1 = [ & ] () -> char {
        rxt_0::linspace_n( grid_x.data(), NX, -2.0, 2.0 );
        rxt_0::linspace_n( grid_y.data(), NY, -3.0, 3.0 );

        for( int y = 0; y < NY; ++y ) { for( int x = 0; x < NX; ++x ) {
            grid_f[ y*NX + x ] = exp( -2.0*abs( grid_x[x] ) ) + cos( M_PI*grid_y[y]/2.0 );
        } }

        for( int y = 0; y < NY; ++y ) { for( int x = 0; x < NX; ++x ) {
            grid_g[ y*NX + x ] = 0.09276*pow( grid_x[x], 4 )
                                 -
                                 0.4881*pow( grid_x[x], 2 )
                                 +
                                 0.08078*pow( grid_y[y], 4 )
                                 -
                                 0.7813*pow( grid_y[y], 2 )
                                 +
                                 1.414;
        } }

        float _MSE = 0.0;
        for( int i = 0; i < NX*NY; ++i ) _MSE += pow( grid_f[i] - grid_g[i], 2 );
        MSE = 1.0/(NX*NY) * _MSE;

        const int N = NX*NY;
        MSE = 1.0/N * rxt_0::roam_acc_2( grid_f.data(), grid_g.data(), N, 0.0f, [] ( auto rhs, auto lhs ) {
            return pow( rhs - lhs, 2 );
        } );

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        tex = G_imm.cluster().tex_handler().make_tex_from_file( "f(x,y)", cache::BucketHandle_Disable, {}, "C:/Users/bobal/Downloads/warc-1.png" );
        glGenFramebuffers( 1, &fbo );
        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->glidx, 0 );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
             spdlog::error( "FBO not complete!" );

        glBindFramebuffer( GL_FRAMEBUFFER, GL_ZERO );

        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        glViewport( 0, 0, 680, 680 );
        glDisable(GL_DEPTH_TEST);
        glClearColor( .0f, .0f, 1.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
        GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);
        glBindFramebuffer( GL_FRAMEBUFFER, GL_ZERO );


        return 0x0;
    }();

    ImGui::Begin( "Plots" );

    ImPlot::PushColormap( ImPlotColormap_Plasma );
    if( ImPlot::BeginPlot( "f(x,y)", {680,680}, ImPlotFlags_Equal ) ) {
        ImPlot::PlotHeatmap(
            "##hm_1", grid_f.data(), NY, NX,
            0, 0, nullptr, {-2,-3}, {2,3},
            ImPlotHeatmapFlags_None
        );

        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if( ImPlot::BeginPlot( "g(x,y)", {680,680}, ImPlotFlags_Equal ) ) {
        ImPlot::PlotHeatmap(
            "##hm_2", grid_g.data(), NY, NX,
            0, 0, nullptr, {-2,-3}, {2,3},
            ImPlotHeatmapFlags_None
        );

        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

    ImGui::SameLine();

    ImGui::Image( (ImTextureID)tex->glidx, {680,680} );

    ImGui::Separator();
    ImGui::Text( "MSE: %f", MSE );

    ImGui::End(); return A113_OK;
}

int main( int argc, char* argv[] ) {
    init( argc, argv, init_args_t{
        .flags = InitFlags_None
    } );

    G_imm.main( argc, argv, clkwrk::Immersive::config_t{
        .ctx        = nullptr,
        .title      = "Rixory Experimental",
        .srf_bgn_as = clkwrk::Immersive::SrfBeginAs_Hide,
        .init_cb    = [] ( const clkwrk::Immersive::init_cb_args_t& args_ ) static -> status_t {
            ImGui::StyleColorsClassic();
            return A113_OK;
        },
        .loop_cb    = [] ( const clkwrk::Immersive::frame_cb_args_t& args_ ) static -> status_t { return ui_frame( args_ ); }
    } );
}