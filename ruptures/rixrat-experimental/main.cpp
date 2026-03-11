#include <a113/clkwrk/immersive.hpp>
#include <imgui.h>
#include <implot.h>

#include <a113/osp/rixrat.hpp>

#include <lapacke.h>

using namespace std; using namespace a113;

static clkwrk::Immersive Imm;

struct _model_t {
    inline static const char*   BASE_SHADERS[ 5 ]   = {
        R"(
            #version 330 core
            //A113#strid BASE_SHADER_VRTX

            layout (location = 0) in vec3 in_vrtx;
            uniform mat4  unif_PV;
            uniform float unif_ZMIN;
            uniform float unif_ZMAX;

            out vec3 vrtx_color;

            vec3 plasma( float t ) {
                const vec3 c0 = vec3(0.058732, 0.023337, 0.543340);
                const vec3 c1 = vec3(2.176515, 0.238383, 0.753960);
                const vec3 c2 = vec3(-2.689460, -7.455851, 3.110800);
                const vec3 c3 = vec3(6.130348, 42.346188, -28.518855);
                const vec3 c4 = vec3(-11.107436, -82.666311, 60.139848);
                const vec3 c5 = vec3(10.023066, 71.413618, -54.072187);
                const vec3 c6 = vec3(-3.658714, -22.931535, 18.191908);

                return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
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
            //A113#strid BASE_SHADER_FRAG
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
            //A113#strid WIRE_SHADER_VRTX

            layout (location = 0) in vec3 in_vrtx;
            uniform mat4 unif_PV;

            void main() {
                vec3 vrtx = in_vrtx; vrtx.z += 0.01;
                gl_Position = unif_PV * vec4( vrtx, 1.0 );
            }
        )",
        nullptr,
        nullptr,
        nullptr,
        R"(
            #version 330 core
            //A113#strid WIRE_SHADER_FRAG
            out vec4 frag;

            void main() {
                frag = vec4( 0.0, 0.0, 0.0, 1.0 );
            }
        )"
    };

    HVec< imm::pipe_t >   pipeb   = nullptr;
    HVec< imm::pipe_t >   pipew   = nullptr;

    GLuint VAO,VBO,EBO; int count;
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
    static rxt_1::srf_grid_t< float > grid_f{ STEP_SZ, -2.0f, 2.0f, STEP_SZ, -3.0f, 3.0f };
    static rxt_1::srf_grid_t< float > grid_g{ STEP_SZ, -2.0f, 2.0f, STEP_SZ, -3.0f, 3.0f };

    static float MSE = 0.0;

    static auto _do_once_1 = [ & ] () -> char {
        grid_f.apply( [] ( float x, float y ) -> float {
            return exp( -2.0*abs( x ) ) + cos( M_PI*y/2.0 );
        } );

        grid_g.apply( [] ( float x, float y ) -> float {
            return 0.09276*pow( x, 4 ) - 0.4881*pow( x, 2 ) + 0.08078*pow( y, 4 ) - 0.7813*pow( y, 2 ) + 1.414;
        } );
        Imm.cluster().disengage_face_culling();
        MSE = grid_f.MSE_with( grid_g );

        auto [ vbo, ebo ] = grid_f.gen_VBO_and_EBO(); G_model.count = ebo.size();

        glBindVertexArray(G_model.VAO);

        glBindBuffer(GL_ARRAY_BUFFER,G_model.VBO);
        glBufferData(GL_ARRAY_BUFFER,vbo.size()*sizeof(float),vbo.data(),GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,G_model.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,ebo.size()*sizeof(unsigned int),ebo.data(),GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);

        return 0x0;
    }();

    ImGui::Begin( "Plots" );

    ImPlot::PushColormap( ImPlotColormap_Plasma );
    if( ImPlot::BeginPlot( "f(x,y)", {680,680}, ImPlotFlags_Equal ) ) {
        ImPlot::PlotHeatmap(
            "##hm_1", grid_f.raw(), grid_f.yn(), grid_f.xn(),
            0, 0, nullptr, {-2,-3}, {2,3},
            ImPlotHeatmapFlags_None
        );

        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if( ImPlot::BeginPlot( "g(x,y)", {680,680}, ImPlotFlags_Equal ) ) {
        ImPlot::PlotHeatmap(
            "##hm_2", grid_g.raw(), grid_g.yn(), grid_g.xn(),
            0, 0, nullptr, {-2,-3}, {2,3},
            ImPlotHeatmapFlags_None
        );

        ImPlot::EndPlot();
    }
    ImPlot::PopColormap();

    ImGui::Separator();
    ImGui::Text( "MSE: %f", MSE );

    ImGui::End(); 

    if( not ImGui::GetIO().WantCaptureMouse ) {
        static bool dwn = false;
        if( not glfwGetMouseButton( Imm.cluster().handle(), GLFW_MOUSE_BUTTON_RIGHT ) ) { dwn = false; goto l_end; }
    
        static double l_y = 0;
        double x, y; glfwGetCursorPos( Imm.cluster().handle(), &x, &y );

        if( not dwn ) {
            dwn = true;
            l_y = x;
        }

        G_lens.yaw_dg( ( x - l_y ) / 12.0 );
        l_y = x;
    l_end:
    }

    const glm::mat4 PV = glm::perspective( (float)M_PI/3, 1.0f, 0.1f, 100.0f ) * G_lens.view();

    glBindVertexArray(G_model.VAO);

    G_model.pipew->use_program();
    G_model.pipew->upload_unif( "unif_PV", PV );

    Imm.cluster().mode_wireframe();
    glDrawElements(GL_TRIANGLES, G_model.count, GL_UNSIGNED_INT, 0);

    G_model.pipeb->use_program();
    G_model.pipeb->upload_unif( "unif_PV", PV );
    G_model.pipeb->upload_unif( "unif_ZMIN", grid_f.min() );
    G_model.pipeb->upload_unif( "unif_ZMAX", grid_f.max() );

    Imm.cluster().mode_fill();
    glDrawElements(GL_TRIANGLES, G_model.count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

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
            G_lens = glm::vec3{ 0, 0, 6 };
            G_lens >= glm::vec3{ 0, 0, 0 };
            G_lens ^= glm::vec3{ 0, 1, 0 };

            return A113_OK;
        },
        .loop_cb    = [] ( const clkwrk::Immersive::frame_cb_args_t& args_ ) static -> status_t { return ui_frame( args_ ); }
    } );
}