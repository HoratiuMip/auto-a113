/**
 * @file: osp/hyper_net.cpp
 * @brief: Implementation file.
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/clkwrk/immersive.hpp>

namespace a113::clkwrk {

void Immersive::_init_assets( void ) {
/* === idle_splash === */ {
    auto& idle_splash = _assets.idle_splash;

    glGenVertexArrays( 1, &idle_splash.VAO );
    glGenBuffers     ( 1, &idle_splash.VBO );
    glGenBuffers     ( 1, &idle_splash.EBO );

    glBindVertexArray( idle_splash.VAO );

    glBindBuffer( GL_ARRAY_BUFFER, idle_splash.VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( idle_splash.vrtx ), idle_splash.vrtx, GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, idle_splash.EBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( idle_splash.idx ), idle_splash.idx, GL_STATIC_DRAW );

    glVertexAttribPointer    ( 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0 );
    glEnableVertexAttribArray( 0 );

    glBindBuffer     ( GL_ARRAY_BUFFER, GL_NONE );
    glBindVertexArray( GL_NONE );

    const char* shaders[ 5 ] = {
        R"(
            #version 410 core
            //A113#strid<a113/immersive/idle_splash.vert>

            layout ( location = 0 ) in vec3 vrtx;
            out vec2 uv;
            void main() {
                gl_Position = vec4( vrtx, 1.0 );
                uv = vrtx.xy;
            }
        )",
        nullptr, nullptr, nullptr,
        R"(
            #version 410 core
            //A113#strid<a113/immersive/idle_splash.frag>

            uniform float rtc;
            in      vec2  uv;
            out     vec4  frag;

            vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
            vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}
            float cnoise(vec2 P){
                vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
                vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
                Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
                vec4 ix = Pi.xzxz;
                vec4 iy = Pi.yyww;
                vec4 fx = Pf.xzxz;
                vec4 fy = Pf.yyww;
                vec4 i = permute(permute(ix) + iy);
                vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
                vec4 gy = abs(gx) - 0.5;
                vec4 tx = floor(gx + 0.5);
                gx = gx - tx;
                vec2 g00 = vec2(gx.x,gy.x);
                vec2 g10 = vec2(gx.y,gy.y);
                vec2 g01 = vec2(gx.z,gy.z);
                vec2 g11 = vec2(gx.w,gy.w);
                vec4 norm = 1.79284291400159 - 0.85373472095314 * 
                    vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
                g00 *= norm.x;
                g01 *= norm.y;
                g10 *= norm.z;
                g11 *= norm.w;
                float n00 = dot(g00, vec2(fx.x, fy.x));
                float n10 = dot(g10, vec2(fx.y, fy.y));
                float n01 = dot(g01, vec2(fx.z, fy.z));
                float n11 = dot(g11, vec2(fx.w, fy.w));
                vec2 fade_xy = fade(Pf.xy);
                vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
                float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
                return 2.3 * n_xy;
            }

            void main() {
                float d = 36.0;
                float t = rtc / 3.0 / d;
                float n = cnoise( vec2( cos(t)*36.0, sin(t)*36.0 ) + uv*1.6 );
                float g = n * 0.1;
                float b = n * 0.32;

                frag = vec4( 0.0, g, b, 1.0 );
            }
        )"
    };

    idle_splash.pipe = _cluster->pipe_handler().make_pipe_from_sources( shaders );
}
}

void Immersive::_clean_assets( void ) {
/* === idle_splash === */ {
    auto& idle_splash = _assets.idle_splash;

    if( idle_splash.VAO != GL_NONE ) glDeleteVertexArrays( 1, &idle_splash.VAO );
    if( idle_splash.VBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.VBO );
    if( idle_splash.EBO != GL_NONE ) glDeleteBuffers     ( 1, &idle_splash.EBO );

    idle_splash.pipe.reset();
}
}

status_t Immersive::assets_idle_splash_render( void ) {
    _cluster->disengage_depth_test();
    _cluster->mode_fill();

    _assets.idle_splash.pipe->use_program();
    _assets.idle_splash.pipe->upload_unif( "rtc", (float)glfwGetTime() );

    glBindVertexArray( _assets.idle_splash.VAO );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    _cluster->engage_depth_test();
    return A113_OK;
}

status_t Immersive::main( int argc_, char* argv_[], const config_t& config_ ) {
    config = config_;

    _logger = spdlog::stdout_color_mt( A113_VERSION_STRING"/immersive" );
    _logger->set_pattern( A113_SPDLOG_PATTERN );
    _logger->info( "main: starting up..." );

    glfwInit();
    glewInit();

    static auto* _static_logger_ptr_ = _logger.get(); 
    glfwSetErrorCallback( [] ( int err_, const char* desc_ ) static -> void {
        _static_logger_ptr_->error( "main: glfw says [{}]: \"{}\".", err_, desc_ );
    } );

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_RESIZABLE, GL_TRUE );
    glfwWindowHint( GLFW_DECORATED, GL_TRUE );
    if( SrfBeginAs_Maximize == config.srf_bgn_as ) glfwWindowHint( GLFW_MAXIMIZED, GL_TRUE );
    
    GLFWwindow* window = glfwCreateWindow( config.width, config.height, config.title, nullptr, nullptr );

    A113_ASSERT_OR( window ) { _logger->error( "main: bad window handle." ); return A113_ERR_EXCOMCALL; }
    _logger->info( "main: window handle OK." );

    glfwMakeContextCurrent( window );
    
    if( not _cluster ) _cluster = HVec< imm::Cluster >::make( imm::Cluster::init_args_t{
        .glfwnd = window
    } );

    glfwSetWindowUserPointer( _cluster->handle(), (void*)this );

    glfwSetFramebufferSizeCallback( _cluster->handle(), [] ( GLFWwindow* wnd_, int w_, int h_ ) static -> void {
        glViewport( 0, 0, w_, h_ );
    } );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL( window, true );
    ImGui_ImplOpenGL3_Init();

    imgui.io  = &ImGui::GetIO();
    imgui.stl = &ImGui::GetStyle();

    _logger->info( "main: imgui OK." );
    
    if     ( SrfBeginAs_Iconify == config.srf_bgn_as ) glfwIconifyWindow( window );
    else if( SrfBeginAs_Hide    == config.srf_bgn_as ) glfwHideWindow( window );

    this->_init_assets();
    _logger->info( "main: init assets OK." );

    if( config.init_cb ) {
        _logger->info( "main: invoke init callback..." );
        A113_ASSERT_OR( A113_OK == this->config.init_cb( init_cb_args_t{
            .ctx = config.ctx
        } ) ) {
            _logger->error( "main: aborted by init callback." );
            return A113_ERR_USERCALL;
        } else {
            _logger->info( "main: init callback OK." );
        }
    } else {
        _logger->info( "main: no init callback to invoke." );
    }
    
    _logger->info( "main: init OK. Diving the loop..." );

    glViewport( 0, 0, config.width, config.height );

    _is_running.store( true, std::memory_order_release );
    while( _is_running.load( std::memory_order_relaxed ) && !glfwWindowShouldClose( window ) ) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        A113_ASSERT_OR( A113_OK == this->config.loop_cb( frame_cb_args_t{
            .ctx = config.ctx,
            .t   = glfwGetTime(),
            .dt  = imgui.io->DeltaTime
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

    _logger->info( "main: shutting down..." );

    if( config.exit_cb ) config.exit_cb( {
        .ctx = config.ctx
    } );

    this->_clean_assets();
    _logger->info( "main: clean assets OK." );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow( window );

    _logger->info( "main: shutdown OK." );
    return A113_OK;
}

}
