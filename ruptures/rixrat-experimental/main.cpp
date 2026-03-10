#include <a113/clkwrk/immersive.hpp>
#include <imgui.h>
#include <implot.h>

#include <a113/osp/rixrat_core.hpp>

#include <lapacke.h>

using namespace std; using namespace a113;

static clkwrk::Immersive Imm;

struct _model_t {
    inline static const char*   BASE_SHADERS[ 5 ]   = {
        R"(
            #version 330 core
            //A113#strid BASE_SHADER_VRTX

            layout (location = 0) in vec3 in_vrtx;

            out vec3 color;

            uniform mat4 unif_PV;

            void main() {
                color       = in_vrtx;
                gl_Position = unif_PV * vec4( in_vrtx, 1.0 );
            }
        )",
        nullptr,
        nullptr,
        nullptr,
        R"(
            #version 330 core
            //A113#strid BASE_SHADER_FRAG

            in vec3 color;

            out vec4 FragColor;

            void main() {
                FragColor = vec4(color,1.0);
            }
        )"
    };

    HVec< imm::pipe_t >   pipe   = nullptr;

    inline static float vertices[] = {
        1.0f,1.0f,1.0f,
        0.5f,1.0f,1.0f,
        0.5f, 0.5f,1.0f,
        1.0f, 0.5f,1.0f,
        1.0f,1.0f, 0.5f,
        0.5f,1.0f, 0.5f,
        0.5f, 0.5f, 0.5f,
        1.0f, 0.5f, 0.5f
    };
    inline static unsigned int indices[] = {
        0,1,2, 2,3,0, // back
        4,5,6, 6,7,4, // front
        0,4,7, 7,3,0, // left
        1,5,6, 6,2,1, // right
        3,2,6, 6,7,3, // top
        0,1,5, 5,4,0  // bottom
    };
    GLuint VAO,VBO,EBO;
    void init( void ) {
        pipe = Imm.cluster().pipe_handler().make_pipe_from_sources( BASE_SHADERS );

        glGenVertexArrays(1,&VAO);
        glGenBuffers(1,&VBO);
        glGenBuffers(1,&EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
    }
}; static _model_t G_model;

static imm::lens_t G_lens;

status_t ui_frame( const clkwrk::Immersive::frame_cb_args_t& args_ ) {
    static rxt_1::srf_grid_t< float > grid_f{ 0.01f, -2.0f, 2.0f, 0.01f, -3.0f, 3.0f };
    static rxt_1::srf_grid_t< float > grid_g{ 0.01f, -2.0f, 2.0f, 0.01f, -3.0f, 3.0f };

    static float MSE = 0.0;

    static auto _do_once_1 = [ & ] () -> char {
        grid_f.apply( [] ( float x, float y ) -> float {
            return exp( -2.0*abs( x ) ) + cos( M_PI*y/2.0 );
        } );

        grid_g.apply( [] ( float x, float y ) -> float {
            return 0.09276*pow( x, 4 ) - 0.4881*pow( x, 2 ) + 0.08078*pow( y, 4 ) - 0.7813*pow( y, 2 ) + 1.414;
        } );

        MSE = grid_f.MSE_with( grid_g );

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

        G_lens.yaw( ( x - l_y ) / 100.0 );
        l_y = x;
    l_end:
    }

    const glm::mat4 PV = glm::perspective( (float)M_PI/3, 1.0f, 0.1f, 100.0f ) * G_lens.view();
    G_model.pipe->upload_unif( "unif_PV", PV );
    
    G_model.pipe->use_program();
    glBindVertexArray(G_model.VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
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