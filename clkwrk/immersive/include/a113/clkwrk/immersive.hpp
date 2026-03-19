#pragma once

#include <a113/osp/render3.hpp>
#include <a113/osp/tempo.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <ImGuiFileDialog.h>

namespace a113::clkwrk {

class Immersive  {
public:
    struct frame_cb_args_t {
        void*    ctx;
        double   dt;
    };

    struct init_cb_args_t {
        void*   ctx;
    };

public:
    typedef   std::function< status_t( const frame_cb_args_t& ) >   frame_callback_t;
    typedef   std::function< status_t( const init_cb_args_t&  ) >   init_callback_t;          

public:
    enum SrfBeginAs_ {
        SrfBeginAs_Default, SrfBeginAs_Iconify, SrfBeginAs_Maximize, SrfBeginAs_Hide
    };

public:
    struct config_t {
        void*                   ctx           = nullptr;

        const char*             title         = A113_VERSION_STRING"::clkwrk::immersive";
        int                     width         = 64;
        int                     height        = 64;
        SrfBeginAs_             srf_bgn_as    = SrfBeginAs_Default;
        glm::vec4               clear_color   = { 0.0, 0.0, 0.0, 1.0 };
        std::filesystem::path   icon_path     = {};

        init_callback_t         init_cb       = nullptr;
        frame_callback_t        loop_cb       = nullptr;

    } config;

public:
    HVec< imm::Cluster >   _cluster      = nullptr;
    imm::lens_t            _lens_0       = { glm::vec3{0,0,5}, glm::vec3{0,0,0}, glm::vec3{0,1,0} };

_A113_PROTECTED:
    std::atomic_bool       _is_running   = false;

public:
    A113_inline imm::Cluster& cluster( void ) {
        return *_cluster;
    }

    A113_inline imm::lens_t lens_0( void ) {
        return _lens_0;
    }

    A113_inline auto native_window_handle( void ) {
        return _cluster->handle();
    }

public:
    status_t set_icon( GLFWimage& img_ ) {
        glfwSetWindowIcon( _cluster->handle(), 1, &img_ );
        return A113_OK;
    }

public:
    status_t main( int argc_, char* argv_[], const config_t& config_ ) {
        config = config_;

        A113_LOGI_IMM( "Immersive clockwork launched. Initializing the graphics library..." );

        glfwInit();
        glewInit();

        glfwSetErrorCallback( [] ( int err_, const char* desc_ ) static -> void {
            static std::string prev_glfw_err_str = "";
            if( prev_glfw_err_str == desc_ ) return;
            prev_glfw_err_str = desc_;
            A113_LOGE_IMM( "GLFW error [{}] occured: \"{}\".", err_, prev_glfw_err_str );
        } );

        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
        glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
        glfwWindowHint( GLFW_DECORATED, GL_TRUE );
        if( SrfBeginAs_Maximize == config.srf_bgn_as ) glfwWindowHint( GLFW_MAXIMIZED, GL_TRUE );
        
        GLFWwindow* window = glfwCreateWindow( config.width, config.height, config.title, nullptr, nullptr );

        A113_ASSERT_OR( window ) { A113_LOGE_IMM_INT( A113_ERR_EXCOMCALL, "Bad window handle." ); return A113_ERR_EXCOMCALL; }
        A113_LOGI_IMM( "Window handle ok." );

        glfwMakeContextCurrent( window );
        
        if( not _cluster ) _cluster = HVec< imm::Cluster >::make( imm::Cluster::init_args_t{
            .glfwnd = window
        } );

        glfwSetWindowUserPointer( _cluster->handle(), config.ctx );

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        struct {
            ImGuiIO&      io      = ImGui::GetIO();
            ImGuiStyle&   style   = ImGui::GetStyle();
        } imgui;

        imgui.io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        imgui.style.WindowRounding = 0.0f; 
        imgui.style.Colors[ ImGuiCol_WindowBg ].w = 1.0f;
        imgui.style.WindowPadding = { 10, 10 };

        ImGui_ImplGlfw_InitForOpenGL( window, true );
        ImGui_ImplOpenGL3_Init();

        A113_LOGI_IMM( "Immediate mode Graphical User Interface ( ImGui ) initialization complete." );
        
        if     ( SrfBeginAs_Iconify == config.srf_bgn_as ) glfwIconifyWindow( window );
        else if( SrfBeginAs_Hide    == config.srf_bgn_as ) glfwHideWindow( window );

        if( config.init_cb ) {
            A113_LOGI_IMM( "Invoking the initialization complete callback..." );
            A113_ASSERT_OR( A113_OK == this->config.init_cb( init_cb_args_t{
                .ctx = config.ctx
            } ) ) {
                A113_LOGE_IMM_INT( A113_ERR_USERCALL, "The immersive clockwork has been aborted by the user from the initialization callback." );
                return A113_ERR_USERCALL;
            }
        } else {
            A113_LOGI_IMM( "No initialization complete callback found." );
        }
        
        A113_LOGI_IMM( "Immersive clockwork initialization complete. Launching the loop." );

        _is_running.store( true, std::memory_order_release );
        while( _is_running.load( std::memory_order_relaxed ) && !glfwWindowShouldClose( window ) ) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if( config.clear_color.a != 0.0f ) _cluster->clear( config.clear_color );

            A113_ASSERT_OR( A113_OK == this->config.loop_cb( frame_cb_args_t{
                .ctx = config.ctx,
                .dt  = imgui.io.DeltaTime
            } ) ) _is_running.store( false, std::memory_order_seq_cst );

            ImGui::Render();

            ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent( window );
            _cluster->swap();
        }

    l_end:
        _is_running.store( false, std::memory_order_seq_cst );

        A113_LOGI_IMM( "Shutting down the immersive clockwork..." );

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        glfwDestroyWindow( window );

        A113_LOGI_IMM( "The immersive clockwork has been shut down completely." );
        return A113_OK;
    }
};

}