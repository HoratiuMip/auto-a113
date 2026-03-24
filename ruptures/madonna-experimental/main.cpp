#include <a113/clkwrk/immersive.hpp>
#include <imgui.h>
#include <implot.h>

#include <a113/osp/madonna.hpp>

using namespace std; using namespace a113;

static clkwrk::Immersive Imm;

struct _model_t {
    inline static const char*   BASE_SHADERS[ 5 ]   = {
        R"(
            #version 330 core
            //A113#strid<BASE_SHADER_VRTX>

            layout (location = 0) in vec3 in_vrtx;
            uniform mat4  unif_PV;
            uniform float unif_ZMIN;
            uniform float unif_ZMAX;

            out vec3 vrtx_color;

            vec3 plasma( float t ) {
                const vec3 c0 = vec3(0.277727, 0.005407, 0.334099);
                const vec3 c1 = vec3(0.105093, 1.404613, 1.384590);
                const vec3 c2 = vec3(-0.330861, 0.214847, 0.095095);
                const vec3 c3 = vec3(-4.634230, -5.799100, -19.332440);
                const vec3 c4 = vec3(6.228269, 14.179933, 56.690552);
                const vec3 c5 = vec3(4.776384, -13.745145, -65.353032);
                const vec3 c6 = vec3(-5.435455, 4.645852, 26.312435);

                return c0 + t*(c1 + t*(c2 + t*(c3 + t*(c4 + t*(c5 + t*c6)))));
            }

            void main() {
                vrtx_color = plasma( 1.0 - (unif_ZMAX - in_vrtx.z) / (unif_ZMAX - unif_ZMIN) );
                gl_Position = unif_PV * vec4( in_vrtx, 1.0 );
            }
        )",
        nullptr,
        nullptr,
        nullptr,
        R"(
            #version 330 core
            //A113#strid<BASE_SHADER_FRAG>
            out vec4 frag;

            in vec3 vrtx_color;

            void main() {
                frag = vec4( vrtx_color, 1.0 );
            }
        )"
    };
    inline static const char*   WIRE_SHADERS[ 5 ]   = {
        R"(
            #version 330 core
            //A113#strid<WIRE_SHADER_VRTX>

            layout (location = 0) in vec3 in_vrtx;
            uniform mat4  unif_PV;
            uniform float unif_off;

            void main() {
                vec3 vrtx = in_vrtx; vrtx.z += unif_off;
                gl_Position = unif_PV * vec4( vrtx, 1.0 );
            }
        )",
        nullptr,
        nullptr,
        nullptr,
        R"(
            #version 330 core
            //A113#strid<WIRE_SHADER_FRAG>
            out vec4 frag;

            void main() {
                frag = vec4( 0.0, 0.0, 0.0, 1.0 );
            }
        )"
    };

    HVec< imm::pipe_t >   pipeb   = nullptr;
    HVec< imm::pipe_t >   pipew   = nullptr;

    GLuint VAO,VBO,EBO;
    int count;

    imm::ren_target_t ren_targ;
    HVec< imm::tex_t > tex;
    void init( void ) {
        pipeb = Imm.cluster().pipe_handler().make_pipe_from_sources( BASE_SHADERS );
        pipew = Imm.cluster().pipe_handler().make_pipe_from_sources( WIRE_SHADERS );

        glGenVertexArrays(1,&VAO);
        glGenBuffers(1,&VBO);
        glGenBuffers(1,&EBO);
    }
}; static _model_t G_model;

static imm::lens_t G_lens;

status_t ui_frame( const clkwrk::Immersive::frame_cb_args_t& args_ ) {
    static constexpr float STEP_SZ = 0.1f;
    static mdn_2::srf_grid_t< float > grid_f;

    static float MSE = 0.0;

    static mdn_1::tfc_t< float > tf{ { 25 }, { 1, 2, 25 }, mdn_0::DiscDiffMth_FwdEuler };
    static auto t = mdn_1::linspace_s<float>( 0.01, 0.0, 15.0 );
    static std::vector< float > y;

    static auto _do_once_1 = [ & ] () -> char {
        grid_f
        .span_s( { {STEP_SZ, -5.0f, 5.0f}, {STEP_SZ, -5.0f, 5.0f} } )
        .apply( [] ( float* x ) -> float {
            return exp(-0.1 * (x[0] * x[0] + x[1] * x[1])) * sin(2 * x[0]) * cos(2 * x[1]);
        } );
      
        auto [ vbo, ebo ] = grid_f.gen_VBO_and_EBO(); G_model.count = ebo.size();

        glBindVertexArray(G_model.VAO);

        glBindBuffer(GL_ARRAY_BUFFER,G_model.VBO);
        glBufferData(GL_ARRAY_BUFFER,vbo.size()*sizeof(float),vbo.data(),GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,G_model.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,ebo.size()*sizeof(unsigned int),ebo.data(),GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
        glEnableVertexAttribArray( GL_ZERO );
        glBindVertexArray( GL_ZERO );
    
        Imm.cluster().disengage_face_culling();

        y.assign( t.size(), 0x0 );
        for( int i = 0x0; i < y.size(); ++i ) {
            y[ i ] = tf.step( 1.0, 0.01 );
        }

        return 0x0;
    }();

    const glm::mat4 PV = glm::perspective( (float)M_PI/3, 1.0f, 0.1f, 1000.0f ) * G_lens.view();

    Imm.assets_idle_splash_render();

    Imm.cluster().push_render_target( &G_model.ren_targ );
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glBindVertexArray(G_model.VAO);

    G_model.pipew->use_program();
    G_model.pipew->upload_unif( "unif_PV", PV );
    G_model.pipew->upload_unif( "unif_off", 0.01 );
    Imm.cluster().mode_wireframe();
    glDrawElements(GL_TRIANGLES, G_model.count, GL_UNSIGNED_INT, 0);
    G_model.pipew->upload_unif( "unif_off", -0.01 );
    glDrawElements(GL_TRIANGLES, G_model.count, GL_UNSIGNED_INT, 0);

    G_model.pipeb->use_program();
    G_model.pipeb->upload_unif( "unif_PV", PV );
    G_model.pipeb->upload_unif( "unif_ZMIN", grid_f.min() );
    G_model.pipeb->upload_unif( "unif_ZMAX", grid_f.max() );
    Imm.cluster().mode_fill();
    glDrawElements(GL_TRIANGLES, G_model.count, GL_UNSIGNED_INT, 0);
    glBindVertexArray( GL_ZERO );

    Imm.cluster().pop_render_target();

    ImGui::Begin( "Plots" );

    ImPlot::PushColormap( ImPlotColormap_Viridis );
    if( ImPlot::BeginPlot( "tf", {0,0}, ImPlotFlags_Equal | ImPlotFlags_Crosshairs ) ) {  
        ImPlot::SetNextLineStyle( { 0.0, 1.0, 1.0, 1.0 }, 2.5 );  
        ImPlot::PlotLine(
            "##tf_1", t.data(), y.data(), t.size()
        );
        ImPlot::EndPlot();
    }

    // if( ImPlot::BeginPlot( "f(x,y)", {680,680}, ImPlotFlags_Equal | ImPlotFlags_Crosshairs ) ) {
    //     ImPlot::SetupAxis( ImAxis_Y1, nullptr, ImPlotAxisFlags_Invert );
        
    //     ImPlot::PlotHeatmap(
    //         "##hm_1", grid_f.raw(), grid_f.n_of(1), grid_f.n_of(0),
    //         0, 0, nullptr, {-5,-5}, {5,5},
    //         ImPlotHeatmapFlags_None
    //     );
    //     ImPlot::EndPlot();
    // }
    ImGui::SameLine();
    ImPlot::ColormapScale(
        "Scale",
        grid_f.min(),
        grid_f.max()
    );
    ImPlot::PopColormap();

    ImGui::SameLine();
    ImGui::Image( (ImTextureID)G_model.ren_targ._tex_glidx, {680,680} );

    static mdn_1::rvec< float > sin_t{500};
    static mdn_1::rvec< float > sin_y{500};

    ImGui::Separator();
    if( ImPlot::BeginPlot( "sin", {0,0}, ImPlotFlags_Equal | ImPlotFlags_Crosshairs ) ) {  
        ImPlot::SetNextLineStyle( { 1.0, 0.32, 0.0, 1.0 }, 2.5 );  
        ImPlot::PlotLine(
            "##sin_1", sin_t.data(), sin_y.data(), sin_t.size(), ImPlotLineFlags_Shaded, sin_t.head()
        );
        ImPlot::EndPlot();
    }

    float ttt = glfwGetTime();
    sin_t.push_back( ttt );
    sin_y.push_back( std::sin( ttt ) );

    ImGui::End();

    if( not ImGui::GetIO().WantCaptureMouse ) {
        static bool dwn = false;
        if( not glfwGetMouseButton( Imm.cluster().handle(), GLFW_MOUSE_BUTTON_RIGHT ) ) { dwn = false; goto l_end; }
    
        static double l_x, l_y = 0;
        double x, y; glfwGetCursorPos( Imm.cluster().handle(), &x, &y );

        if( not dwn ) {
            dwn = true;
            l_x = x;
            l_y = y;
        }

        G_lens.pitch_dl( ( y - l_y ) / 12.0 );
        G_lens.yaw_dl( ( x - l_x ) / 12.0 );
        l_x = x; l_y = y;
    l_end:
    }

    return A113_OK;
}

#include <iostream>
int main( int argc, char* argv[] ) {
    init( argc, argv, init_args_t{
        .flags = InitFlags_None
    } );

    Imm.main( argc, argv, clkwrk::Immersive::config_t{
        .ctx        = nullptr,
        .title      = "RixRat-Exp",
        .width      = 680,
        .height     = 680,
        .srf_bgn_as = clkwrk::Immersive::SrfBeginAs_Default,
        .init_cb    = [] ( const clkwrk::Immersive::init_cb_args_t& args_ ) static -> status_t {
            ImGui::StyleColorsClassic();

            G_model.init();
            G_lens = glm::vec3{ 0, 0, 10 };
            G_lens >= glm::vec3{ 0, 0, 0 };
            G_lens ^= glm::vec3{ 0, 1, 0 };

            G_model.ren_targ.bind( 680, 680 );
            
            return A113_OK;
        },
        .loop_cb    = [] ( const clkwrk::Immersive::frame_cb_args_t& args_ ) static -> status_t { return ui_frame( args_ ); }
    } );
}