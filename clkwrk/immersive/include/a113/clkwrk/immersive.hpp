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
        double   t;
        double   dt;
    };
    struct init_cb_args_t {
        void*   ctx;
    };
    struct exit_cb_args_t {
        void*   ctx;
    };

public:
    typedef   std::function< status_t( const frame_cb_args_t& ) >   frame_callback_t;
    typedef   std::function< status_t( const init_cb_args_t&  ) >   init_callback_t;   
    typedef   std::function< void( const exit_cb_args_t& ) >        exit_callback_t;       

public:
    enum SrfBeginAs_ {
        SrfBeginAs_Default, SrfBeginAs_Iconify, SrfBeginAs_Maximize, SrfBeginAs_Hide
    };

public:
    struct config_t {
        void*                   ctx           = nullptr;

        const char*             title         = A113_VERSION_STRING"/Immersive";
        int                     width         = 64;
        int                     height        = 64;

        SrfBeginAs_             srf_bgn_as    = SrfBeginAs_Default;

        std::filesystem::path   icon_path     = {};

        init_callback_t         init_cb       = nullptr;
        frame_callback_t        loop_cb       = nullptr;
        exit_callback_t         exit_cb       = nullptr;

    } config;

public:
    HVec< imm::Cluster >     _cluster      = nullptr;
    imm::lens_t              _lens_0       = { glm::vec3{0,0,5}, glm::vec3{0,0,0}, glm::vec3{0,1,0} };

_A113_PROTECTED:
    HVec< spdlog::logger >   _logger       = nullptr;

_A113_PROTECTED:
    std::atomic_bool         _is_running   = false;

public:
    struct imgui_t {
        ImGuiIO*      io    = nullptr;
        ImGuiStyle*   stl   = nullptr;
    } imgui;

_A113_PROTECTED:
    struct _assets_t {
        /**
         * @brief A perlin noise splasher.
         * @details Perlin by XorDev: https://x.com/XorDev/status/1894123951401378051.
         */
        struct idle_splash_t {
            inline static float          vrtx[]   = { 1,1, 1,-1, -1,-1, -1,1 };
            inline static unsigned int   idx[]    = { 0,1,3, 1,2,3 };
            GLuint                       VAO      = GL_NONE;
            GLuint                       VBO      = GL_NONE;
            GLuint                       EBO      = GL_NONE;
            HVec< imm::pipe_t >          pipe     = nullptr;
        } idle_splash;
    } _assets;

    void _init_assets( void );
    void _clean_assets( void );

public:
    status_t assets_idle_splash_render( void );

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
    status_t main( int argc_, char* argv_[], const config_t& config_ );
};

}