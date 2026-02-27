#pragma once
/**
 * @file: osp/render3.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>
#include <a113/osp/cache.hpp>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tiny_obj_loader.h>


namespace a113::imm {

enum ShaderPhase_ {
    ShaderPhase_None     = -0x1,

    ShaderPhase_Vertex   = 0x0,
    ShaderPhase_TessCtrl = 0x1,
    ShaderPhase_TessEval = 0x2,
    ShaderPhase_Geometry = 0x3,
    ShaderPhase_Fragment = 0x4,

    ShaderPhase_COUNT    = 0x5
};

inline constexpr const GLuint ShaderPhase_MAP[ ShaderPhase_COUNT ] = {
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER
};

inline constexpr const char* const ShaderPhase_FILE_EXTENSION[ ShaderPhase_COUNT ] = {
    ".vert", ".tesc", ".tese", ".geom", ".frag"
};


enum MeshFlag_ {
    MeshFlag_MakePipe = _BV( 0x0 )
};

struct tex_params_t {
    GLuint   min_filter    = GL_LINEAR_MIPMAP_LINEAR;
    GLuint   mag_filter    = GL_LINEAR;
    GLuint   v_flip        = false;
    bool     keep_in_RAM   = false;
};
struct tex_t {
    tex_t( void ) = default;
    tex_t( const std::string& strid_, GLuint glidx_ ) : strid{ strid_ }, glidx{ glidx_ } {}

    tex_t( const tex_t& ) = delete;
    tex_t( tex_t&& other_ ) : strid{ std::move( other_.strid ) }, glidx{ std::exchange( other_.glidx, GL_NONE ) } {}

    ~tex_t( void ) { 
        A113_ASSERT_OR( GL_NONE != glidx ) return;
        glDeleteTextures( 1, &glidx );
        glidx = GL_NONE; 

        if( RAM_img.pixels ) free( RAM_img.pixels );
    }

    std::string   strid     = {};
    GLuint        glidx     = GL_NONE;
    GLFWimage     RAM_img   = {};
};

class Cluster {
public:
    /* MIP here after almost one year. Yeah.*/
    //std::cout << "GOD I SUMMON U. GIVE MIP TEO FOR A FEW DATES (AT LEAST 100)"; 
    //std::cout << "TY";

public:
    struct init_args_t {
        GLFWwindow*   glfwnd;
    };

public:
    Cluster( const init_args_t& args_ )
    :  _glfwnd{ args_.glfwnd } 
    {
        glfwMakeContextCurrent( _glfwnd );

        _rend_str = ( const char* )glGetString( GL_RENDERER ); 
        _gl_str   = ( const char* )glGetString( GL_VERSION );

        glDepthFunc( GL_LESS );
        glEnable( GL_DEPTH_TEST );

        glFrontFace( GL_CCW );

        glCullFace( GL_BACK );
        glEnable( GL_CULL_FACE ); 

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable( GL_BLEND );

        glewExperimental = GL_TRUE; 
        glewInit();

        int wnd_w, wnd_h;
        glfwGetFramebufferSize( _glfwnd, &wnd_w, &wnd_h );
        glViewport( 0, 0, wnd_w, wnd_h );

        A113_LOGI_IMM( "Rendering cluster docked on {}, using {}.", _rend_str ? _rend_str : "NULL", _gl_str ? _gl_str : "NULL" );
    }

    Cluster( const Cluster& ) = delete;
    Cluster( Cluster&& ) = delete;

_A113_PROTECTED:
    GLFWwindow*              _glfwnd     = nullptr;
 
    const char*              _rend_str   = nullptr;     
    const char*              _gl_str     = nullptr;  

_A113_PROTECTED:
    struct _internal_struct_t{ _internal_struct_t( Cluster* Cluster_ ) : _Cluster{ Cluster_ } {} Cluster* _Cluster = nullptr; };

_A113_PROTECTED:
    struct shader_t {
        shader_t( void ) = default;
        shader_t( const std::string& strid_, GLuint glidx_ ) : strid{ strid_ }, glidx{ glidx_ } {}

        shader_t( const shader_t& ) = delete;
        shader_t( shader_t&& other_ ) : strid{ std::move( other_.strid ) }, glidx{ std::exchange( other_.glidx, GL_NONE ) } {}

        ~shader_t( void ) { 
            A113_ASSERT_OR( GL_NONE != glidx ) return;
            glDeleteShader( std::exchange( glidx, GL_NONE ) ); 
        }

        std::string   strid   = {};
        GLuint        glidx   = GL_NONE;
    };
    struct _shader_cache_t : public _internal_struct_t {
        cache::Bucket< std::string, shader_t >   _buckets[ ShaderPhase_COUNT ]   = {};

        HVec< shader_t > make_shader( std::string source_, std::string strid_, ShaderPhase_ phase_, const char* from_ ) {
            if( strid_.empty() ) strid_ = std::to_string( std::hash< std::string >{}( source_ ) );

            auto bkt_hdl_ = cache::BucketHandle_None;
            auto shader = _buckets[ phase_ ].query( strid_, bkt_hdl_ );

            if( not shader ) {
                shader = HVec< shader_t >::make( std::move( strid_ ), glCreateShader( ShaderPhase_MAP[ phase_ ] ) );

                A113_ASSERT_OR( GL_NONE != shader->glidx ) {
                    A113_LOGE_IMM( "Could not create SHADER[{}].", shader->strid );
                    return nullptr;
                }

                const GLchar* const_const_const_const_const_const = source_.c_str();
                glShaderSource( shader->glidx, 1, &const_const_const_const_const_const, NULL );
                glCompileShader( shader->glidx );
            
                GLint status; glGetShaderiv( shader->glidx, GL_COMPILE_STATUS, &status );
                A113_ASSERT_OR( status ) {
                    GLchar log_buf[ 512 ];
                    glGetShaderInfoLog( shader->glidx, sizeof( log_buf ), NULL, log_buf );
                    A113_LOGE_IMM( "Could not compile SHADER[{}]: \"{}\".", shader->strid, log_buf );
                    return nullptr;
                }

                _buckets[ phase_ ].commit( shader->strid, shader );
                A113_LOGI_IMM( "Created SHADER[{}] from {}.", shader->strid, from_ );
            } else {
                A113_LOGI_IMM( "Pulled SHADER[{}] from cache, requested from {}.", shader->strid, from_ );
            }

            return shader;
        }

        HVec< shader_t > make_shader_from_file( const std::filesystem::path& path_, ShaderPhase_ phase_ = ShaderPhase_None ) {
            status_t    status   = A113_OK;
            std::string source   = {};
            std::string line     = {};

            std::string strid    = "";

            if( ShaderPhase_None == phase_ ) {
                int index = ShaderPhase_Vertex;
                for( auto file_ext : ShaderPhase_FILE_EXTENSION ) {
                    if( path_.string().ends_with( file_ext ) ) { phase_ = ( ShaderPhase_ )index; break; }   
                    ++index;
                }     
            }
            
            std::function< void( const std::filesystem::path& ) > accumulate_glsl = [ & ] ( const std::filesystem::path& path_ ) -> void {
                std::ifstream file{ path_, std::ios_base::binary };

                A113_ASSERT_OR( file.operator bool() ) {
                    A113_LOGE_IMM( "Could not open file \"{}\".", path_.string() );
                    status = A113_ERR_OPEN; return;
                }

                while( std::getline( file, line ) ) {
                    struct _directive_t {
                        const char*   str;
                        void*         lbl;
                    } directives[] = {
                        { str: "//A113#include", lbl: &&l_directive_include },
                        { str: "//A113#strid", lbl: &&l_directive_strid }
                    };

                    std::string arg;

                    for( auto& d : directives ) {
                        if( !line.starts_with( d.str ) ) continue;
                        
                        auto q1 = line.find_first_of( '<' );
                        auto q2 = line.find_last_of( '>' );

                        A113_ASSERT_OR( q1 != std::string::npos && q2 != std::string::npos ) {
                            A113_LOGE_IMM( "DIRECTIVE[{}] argument of SHADER[{}] is ill-formed. It shall be quoted between \"<>\". ", d.str, strid );
                            status = -0x1; return;
                        }
                        
                        arg = std::string{ line.c_str() + q1 + 1, q2 - q1 - 1 };
                        goto *d.lbl;
                    }
                    goto l_code_line;
                
                l_directive_include:
                    accumulate_glsl( path_.parent_path() / arg );
                    continue;
                
                l_directive_strid:
                    A113_ASSERT_OR( strid.empty() ) {
                        A113_LOGE_IMM( "Multiple string identifiers given for SHADER[{}]<->[{}].", strid, arg );
                        status = -0x1; return;
                    }
                    strid = std::move( arg );
                    continue;

                l_code_line:
                    source += line; source += '\n';
                }
            };

            accumulate_glsl( path_ );
            
            A113_ASSERT_OR( A113_OK == status ) {
                A113_LOGE_IMM_INT( status, "Fault during accumulation of source code for SHADER[{}].", strid );
                return nullptr;
            }

            return this->make_shader( source, std::move( strid ), phase_, std::format( "\"{}\"", path_.string() ).c_str() );
        }

    } _shader_cache{ this };

    struct pipe_t {
        pipe_t( void ) = default;
        pipe_t( const std::string& strid_, GLuint glidx_ ) : strid{ strid_ }, glidx{ glidx_ } {}

        pipe_t( const pipe_t& ) = delete;
        pipe_t( pipe_t&& other_ ) : strid{ std::move( other_.strid ) }, glidx{ std::exchange( other_.glidx, GL_NONE ) } {}

        ~pipe_t( void ) {
            A113_ASSERT_OR( GL_NONE != glidx ) return;
            glDeleteProgram( std::exchange( glidx, GL_NONE ) );
        }

        std::string   strid   = {};
        GLuint        glidx   = GL_NONE;
    };
    struct _pipe_cache_t : public _internal_struct_t { 
        cache::Bucket< std::string, pipe_t >   _bucket   = {};

        HVec< pipe_t > make_pipe( shader_t* arr_[ ShaderPhase_COUNT ], const char* from_ ) {
            static const char* const stage_pretties[ ShaderPhase_COUNT ] = {
                "Vertex-", ">TessControl-", ">TessEval-", ">Geometry-", ">Fragment"
            };
            std::string pretty = {};
            std::string strid  = {};

            for( int phase = ShaderPhase_Vertex; phase <= ShaderPhase_Fragment; ++phase ) {
                shader_t* shader = arr_[ phase ]; if( not shader ) continue;

                pretty += stage_pretties[ phase ];
                strid += shader->strid + '>';
            }

            auto bkt_hdl_ = cache::BucketHandle_None;
            auto pipe = _bucket.query( strid, bkt_hdl_ );

            if( not pipe ) {
                pipe = HVec< pipe_t >::make( std::move( strid ), glCreateProgram() );

                A113_ASSERT_OR( GL_NONE != pipe->glidx ) {
                    A113_LOGE_IMM( "Could not create create PIPE[{}].", pipe->strid );
                    return nullptr;
                }
    
                for( int phase = ShaderPhase_Vertex; phase <= ShaderPhase_Fragment; ++phase ) {
                    if( not arr_[ phase ] ) continue;
                    glAttachShader( pipe->glidx, arr_[ phase ]->glidx );
                }

                glLinkProgram( pipe->glidx );

                GLint status;
                glGetProgramiv( pipe->glidx, GL_LINK_STATUS, &status );
                A113_ASSERT_OR( GL_FALSE != status ) {
                    GLchar log_buf[ 512 ];
                    glGetProgramInfoLog( pipe->glidx, sizeof( log_buf ), NULL, log_buf );
                    A113_LOGE_IMM( "Could not link PIPE[{}]: \"{}\".", pipe->strid, pipe->glidx, log_buf );
                    return nullptr;
                }

                _bucket.commit( pipe->strid, pipe );
                A113_LOGI_IMM( "Created PIPE[{}] from {}.", pipe->strid, from_ );
            } else {
                A113_LOGI_IMM( "Pulled PIPE[{}] from cache, requested from {}.", pipe->strid, from_ );
            }

            return pipe;
        }

        HVec< pipe_t > make_pipe_from_prefixed_path( const std::filesystem::path& path_ ) {
            shader_t* shaders[ ShaderPhase_COUNT ]; memset( shaders, 0x0, sizeof( shaders ) );

            for( int phase = ShaderPhase_Vertex; phase <= ShaderPhase_Fragment; ++phase ) {
                std::filesystem::path path_phase{ path_ };
                path_phase += ShaderPhase_FILE_EXTENSION[ phase ];

                if( !std::filesystem::exists( path_phase ) ) continue;
        
                shaders[ phase ] = _Cluster->_shader_cache.make_shader_from_file( path_phase, ( ShaderPhase_ )phase ).get();
            }

            return this->make_pipe( shaders, std::format( "\"{}\"", path_.string() ).c_str() );
        }

    } _pipe_cache{ this };

_A113_PROTECTED:
    struct _tex_cache_t : public _internal_struct_t {
        cache::Bucket< std::string, tex_t >   _bucket   = {};

        HVec< tex_t > make_tex( std::string strid_, cache::BucketHandle_ bkt_hdl_, const tex_params_t& params_, const void* pixels_, int x_, int y_, const char* from_ ) {
            HVec< tex_t > tex = _bucket.query( strid_, bkt_hdl_ );

            if( not tex ) {
                GLuint tex_glidx;

                glGenTextures( 1, &tex_glidx );
                glBindTexture( GL_TEXTURE_2D, tex_glidx );
                glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB, x_, y_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_ );
                glGenerateMipmap( GL_TEXTURE_2D );

                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params_.min_filter );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params_.mag_filter );

                glBindTexture( GL_TEXTURE_2D, GL_NONE );

                tex = _bucket.commit( strid_, tex_t{ strid_, tex_glidx }, bkt_hdl_ );
                A113_LOGI_IMM( "Created TEX[{}] from {}.", tex->strid, from_ );
            } else {
                A113_LOGI_IMM( "Pulled TEX[{}] from cache, requested from {}.", tex->strid, from_ );
            }
            
            return tex;
        }

        HVec< tex_t > make_tex_from_file( std::string strid_, cache::BucketHandle_ bkt_hdl_, const tex_params_t& params_, const std::filesystem::path& path_ ) {
            if( strid_.empty() ) strid_ = path_.string();

            auto tex = _bucket.query( strid_, bkt_hdl_ );

            if( not tex ) {
                int x, y, n;
                stbi_set_flip_vertically_on_load( params_.v_flip );
                unsigned char* img_buf = stbi_load( path_.string().c_str(), &x, &y, &n, 4 );

                A113_ASSERT_OR( img_buf ) {
                    A113_LOGE_IMM_INT( A113_ERR_OPEN, "Could NOT load texture from {}.", path_.string() );
                    return nullptr;
                }

                tex = this->make_tex( strid_, bkt_hdl_, params_, img_buf, x, y, path_.string().c_str() );
                if( params_.keep_in_RAM ) {
                    tex->RAM_img.width  = x;
                    tex->RAM_img.height = y;
                    tex->RAM_img.pixels = img_buf;
                } else {
                    stbi_image_free( img_buf );
                }
            } 
            return tex;
        }
    } _tex_cache{ this };

_A113_PROTECTED:
    class mesh_t : public _internal_struct_t {
    public:
        mesh_t() : _internal_struct_t{ nullptr } {}

        mesh_t( 
            const std::filesystem::path& root_dir_, 
            std::string_view             prefix_, 
            MeshFlag_                    flags_ 
        ) : _internal_struct_t{ nullptr } {
            status_t                           status;

            tinyobj::attrib_t                  attrib;
            std::vector< tinyobj::shape_t >    shapes;
            std::vector< tinyobj::material_t > mtls;
            std::string                        error_str;

            size_t                             total_vrtx_count = 0;

            std::filesystem::path root_dir_p = root_dir_ / prefix_.data();
            std::filesystem::path obj_path   = root_dir_p; obj_path += ".obj";

            A113_LOGI_IMM( "Compiling the obj: \"{}\".", obj_path.string() );

            status = tinyobj::LoadObj( 
                &attrib, &shapes, &mtls, &error_str, 
                obj_path.string().c_str(), root_dir_.string().c_str(), 
                true
            );

            if( not error_str.empty() ) A113_LOGW_IMM( "TinyObj says: \"{}\".", error_str );

            A113_ASSERT_OR( status ) {
                A113_LOGE_IMM( "Failed to compile the obj." );
                return;
            }

            A113_LOGI_IMM( "Compiled [{}] materials over [{}] sub-meshes.", mtls.size(), shapes.size() );

            _mtls.reserve( mtls.size() );
            for( tinyobj::material_t& mtl_data : mtls ) { 
                _mtl_t& mtl = _mtls.emplace_back(); 
                mtl.data = std::move( mtl_data ); 

                GLuint tex_slot = 0x0;

                struct _std_tex_t {
                    const char*    key;
                    std::string*   name;
                } std_texs[] = {
                    { "map_Ka", &mtl.data.ambient_texname },
                    { "map_Kd", &mtl.data.diffuse_texname },
                    { "map_Ks", &mtl.data.specular_texname },
                    { "map_Ns", &mtl.data.specular_highlight_texname },
                    { "map_bump", &mtl.data.bump_texname }
                };

                for( auto& [ key, name ] : std_texs ) {
                    if( name->empty() ) continue;

                    A113_ASSERT_OR( 0x0 == this->_push_tex( root_dir_ / *name, key, tex_slot ) ) continue;

                    mtl.tex_idxs.push_back( _texs.size() - 1 );
                    ++tex_slot;
                }
            
                for( auto& [ key, value ] : mtl.data.unknown_parameter ) {
                    if( not key.starts_with( "A113" ) ) {
                        A113_LOGW_IMM( "Unknown parameter: \"{}\".", key );
                        continue;
                    }

                    if( std::string::npos != key.find( "map" ) && 0x0 == this->_push_tex( root_dir_ / value, key, tex_slot ) ) {
                        mtl.tex_idxs.push_back( _texs.size() - 1 );
                        ++tex_slot;
                        continue;
                    }
    
                    A113_LOGW_IMM( "Unrecognized A113 parameter \"{}\".", key );
                }
            }

            for( tinyobj::shape_t& shape : shapes ) {
                tinyobj::mesh_t& mesh = shape.mesh;
                sub_mesh_t&      sub  = _sub_meshes.emplace_back();

                sub.count = mesh.indices.size();

                struct _vrtx_data_t {
                    glm::vec3   pos;
                    glm::vec3   nrm;
                    glm::vec2   txt;
                };
                std::vector< _vrtx_data_t > vrtx_data; vrtx_data.reserve( sub.count );
                size_t base_idx = 0x0;
                size_t v_acc    = 0;
                size_t l_mtl    = mesh.material_ids[ 0 ];
                for( size_t f_idx = 0x0; f_idx < mesh.num_face_vertices.size(); ++f_idx ) {
                    uint8_t f_c = mesh.num_face_vertices[ f_idx ];

                    for( uint8_t v_idx = 0x0; v_idx < f_c; ++v_idx ) {
                        tinyobj::index_t& idx = mesh.indices[ base_idx + v_idx ];

                        vrtx_data.emplace_back( _vrtx_data_t{
                            pos: { *( glm::vec3* )&attrib.vertices[ 3 *idx.vertex_index ] },
                            nrm: { *( glm::vec3* )&attrib.normals[ 3 *idx.normal_index ] },
                            txt: { ( -0x1 != idx.texcoord_index ) ? *( glm::vec2* )&attrib.texcoords[ 2*idx.texcoord_index ] : glm::vec2{ 1.0 } }
                        } );
                    }

                    if( mesh.material_ids[ f_idx ] != l_mtl ) {
                        sub.strokes.emplace_back( sub_mesh_t::stroke_t{ count: v_acc, mtl_idx: l_mtl } );
                        v_acc = 0;
                        l_mtl = mesh.material_ids[ f_idx ];
                    }

                    v_acc    += f_c;
                    base_idx += f_c;
                }
                sub.strokes.emplace_back( sub_mesh_t::stroke_t{ count: v_acc, mtl_idx: l_mtl } );

                glGenVertexArrays( 1, &sub.VAO );
                glGenBuffers( 1, &sub.VBO );

                glBindVertexArray( sub.VAO );
            
                glBindBuffer( GL_ARRAY_BUFFER, sub.VBO );
                glBufferData( GL_ARRAY_BUFFER, vrtx_data.size() * sizeof( _vrtx_data_t ), vrtx_data.data(), GL_STATIC_DRAW );

                glEnableVertexAttribArray( 0x0 );
                glVertexAttribPointer( 0x0, 0x3, GL_FLOAT, GL_FALSE, sizeof( _vrtx_data_t ), ( GLvoid* )0x0 );
                
                glEnableVertexAttribArray( 0x1 );
                glVertexAttribPointer( 0x1, 0x3, GL_FLOAT, GL_FALSE, sizeof( _vrtx_data_t ), ( GLvoid* )offsetof( _vrtx_data_t, nrm ) );
            
                glEnableVertexAttribArray( 0x2 );
                glVertexAttribPointer( 0x2, 0x2, GL_FLOAT, GL_FALSE, sizeof( _vrtx_data_t ), ( GLvoid* )offsetof( _vrtx_data_t, txt ) );

                std::vector< GLuint > indices; indices.assign( sub.count, 0x0 );
                for( size_t idx = 0x1; idx < indices.size(); ++idx ) indices[ idx ] = idx;

                glGenBuffers( 1, &sub.EBO );
                glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, sub.EBO );
                glBufferData( GL_ELEMENT_ARRAY_BUFFER, sub.count * sizeof( GLuint ), indices.data(), GL_STATIC_DRAW );
            }

            glBindVertexArray( 0x0 );

            // for( auto& tex : _texs )
            //     tex.ufrm = Uniform3< glm::u32 >{ tex.name.c_str(), tex.unit, echo };

            // if( flags & MESH3_FLAG_MAKE_PIPES ) {  
            //     this->pipe.vector( GME_render_cluster3.make_pipe_from_prefixed_path( root_dir_p, echo ) );
            //     this->dock_in( nullptr, echo );
            // }
        }

    _A113_PROTECTED:
        struct sub_mesh_t {
            GLuint                    VAO;
            GLuint                    VBO;
            GLuint                    EBO;
            size_t                    count;
            struct stroke_t {
                size_t   count;
                size_t   mtl_idx;
            };
            std::vector< stroke_t >   strokes;
        };
        std::vector< sub_mesh_t >   _sub_meshes;
        struct _mtl_t {
            tinyobj::material_t     data;
            std::vector< size_t >   tex_idxs;
        };
        std::vector< _mtl_t >       _mtls;
        struct _tex_t {
            GLuint                 glidx;
            std::string            name;
            GLuint                 slot;
            //Uniform3< glm::u32 >   ufrm;
        };
        std::vector< _tex_t >       _texs;

    public:
        // Uniform3< glm::mat4 >     model;
        // Uniform3< glm::vec3 >     Kd;

        // HVec< ShaderPipe3 >       pipe;

    _A113_PROTECTED:
        status_t _push_tex( 
            const std::filesystem::path& path_, 
            std::string_view             name_, 
            GLuint                       slot_
        ) {
            GLuint tex_glidx;

            int x, y, n;
            unsigned char* img_buf = stbi_load( path_.string().c_str(), &x, &y, &n, 4 );

            A113_ASSERT_OR( img_buf ) {
                A113_LOGE_IMM( "Failed to load texture from: \"{}\".", path_.string() );
                return -0x1;
            }

            glGenTextures( 1, &tex_glidx );
            glBindTexture( GL_TEXTURE_2D, tex_glidx );
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_SRGB,
                x, y,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                img_buf
            );
            glGenerateMipmap( GL_TEXTURE_2D );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

            glBindTexture( GL_TEXTURE_2D, 0x0 );
            stbi_image_free( img_buf );

            _texs.emplace_back( _tex_t{
                .glidx =  tex_glidx,
                .name  = name_.data(),
                .slot  = slot_//,
                //.ufrm  = {}
            } );

            A113_LOGI_IMM( "Pushed texture on slot [{}], from: \"{}\". ", slot_, path_.string() );
            return 0x0;
        }

    public:

    public:
        // Mesh3& splash( ShaderPipe3& pipe ) {
        //     pipe.uplink();
        //     this->model.uplink();

        //     for( _SubMesh& sub : _sub_meshes ) {
        //         glBindVertexArray( sub.VAO );

        //         for( auto& burst : sub.bursts ) {
        //             float* diffuse = _mtls[ burst.mtl_idx ].data.diffuse;
        //             Kd.uplink_bv( glm::vec3{ diffuse[ 0 ], diffuse[ 1 ], diffuse[ 2 ] } );

        //             for( size_t tex_idx : _mtls[ burst.mtl_idx ].tex_idxs ) {
        //                 if( tex_idx == -1 ) continue;

        //                 _Tex& tex = _texs[ tex_idx ];

        //                 glActiveTexture( GL_TEXTURE0 + tex.unit );
        //                 glBindTexture( GL_TEXTURE_2D, tex.glidx );
        //                 tex.ufrm.uplink();
        //             }
                    
        //             glDrawElements( pipe.draw_mode, ( GLsizei )burst.count, GL_UNSIGNED_INT, 0 );
        //         }
        //     }

        //     return *this;
        // }

    };

public:
    auto& shader_handler( void ) { return _shader_cache; }
    auto& pipe_handler( void ) { return _pipe_cache; }
    auto& tex_handler( void ) { return _tex_cache; }

public:
    void clear( glm::vec4 c = { .0, .0, .0, 1.0 } ) {
        glClearColor( c.r, c.g, c.b, c.a );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    void swap( void ) {
        glfwSwapBuffers( _glfwnd );
    }

public:
    void engage_face_culling( void ) {
        glEnable( GL_CULL_FACE );
    }

    void disengage_face_culling( void ) {
        glDisable( GL_CULL_FACE );
    }

    void mode_fill( void ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }

    void mode_wireframe( void ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    void mode_points( void ) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
    }

public:
    float aspect_ratio( void ) const {
        int wnd_w, wnd_h;
        glfwGetFramebufferSize( _glfwnd, &wnd_w, &wnd_h );
        return ( float )wnd_w / ( float )wnd_h;
    }

public:
    GLFWwindow* handle( void ) { return _glfwnd; }

};


};
