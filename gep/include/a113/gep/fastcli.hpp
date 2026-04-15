#pragma once
/**
 * @file: osp/fastcli.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/gep/core.hpp>

namespace a113::text {

#define A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES case 0x1: return A113_ERR_BADARG;

class Fastcli {
public:
    struct cmd_t;

_A113_PROTECTED:
    /**
     * @brief Used across internal functions while parsing a command.
     */
    struct _parse_ctx_t {
        /* The whole command line. */
        const std::string&           text      = {};
        /* Final accumulated output of the command line execution. */
        std::string*                 out       = nullptr;
        /* Tokens extracted from 'text'. */
        std::vector< std::string >   toks      = {};
        /* Reference to effective command structure. */
        const cmd_t*                 cmd       = nullptr;
        /* Context-wise index 0. Multiple uses. */
        int                          id0       = 0x0;
        /* Current token during splitting 'text'. */
        std::string                  crt_tok   = "";
        /* Flag if we are inside quotes while tokening. */
        bool                         in_qte    = false;
        /* Reference to effective option token. */
        const std::string*           opt_tok   = nullptr;
        /* Reference to effective option argument token. */
        const std::string*           arg_tok   = nullptr; 

        bool push_crt_tok( void ) {
            if( crt_tok.empty() ) return false;
            toks.emplace_back( std::move( crt_tok ) ); return true;
        }
    };

public:
    /**
     * @brief Master configuration of the parser.
     */
    struct config_t {
        std::string   delim_chrs  = " \t\n";
        std::string   xtra_chrs   = ".,_*/+-?!:=~()[]^";
        std::string   esc_chrs    = "\\";
        std::string   qte_chrs    = "\"\'";
        std::string   var_chrs    = "$";
    };
    
    enum Arg_ {
        Arg_flag,
        Arg_text, Arg_lmhi = Arg_text,
        Arg_i32, Arg_f32, Arg_f64
    };
    enum Argc_ {
        Argc_single, Argc_multi_compact
    };
    /**
     * @brief Command option.
     * @details (1): --some-opt (2): --some-opt-w-arg 69 (3): -s (4): -S 69
     *          An option that does not start with neither '-' or '--', is considered the nth option (using fast index).
     */
    struct opt_t {
        char          sh0rt     = '\0';
        std::string   l0ng      = {};
        Arg_          arg       = Arg_flag;
        int           fast_id   = -0x1;
        Argc_         argc      = Argc_single;
    };

    /**
     * @brief Structure passed to the user parse callback.
     * @details Usage is intended to be as close as possible to getopt_long().
     */
    struct stencil_t {
        _parse_ctx_t*   _ctx   = nullptr;
        int             _fid   = 0x0;

        char            _arg_mem[ 32 ];
        void*           _arg   = nullptr;

    public:
        char next( void );
    
    public:
        A113_inline auto& arg_text( void ) { return *(const std::string*)_arg; }
        A113_inline auto  arg_i32( void )  { return *(int32_t*)_arg; }
        A113_inline auto  arg_f32( void )  { return *(float*)_arg; }
        A113_inline auto  arg_f64( void )  { return *(double*)_arg; }

        A113_inline auto& arg_textv( void ) { return *(std::vector< const std::string* >*)_arg; }
        A113_inline auto& arg_i32v( void )  { return *(std::vector< int32_t >*)_arg; }
        A113_inline auto& arg_f32v( void )  { return *(std::vector< float >*)_arg; }
        A113_inline auto& arg_f64v( void )  { return *(std::vector< double >*)_arg; }

    public:
        A113_inline void operator += ( const std::string& line_ ) { *_ctx->out += line_; }

        template< typename ..._Args_ > A113_inline void operator () ( const char* const fmt_, _Args_&&... args_ ) {
            this->operator+=( std::vformat( fmt_, std::make_format_args( std::forward< _Args_ >( args_ )... ) ) );
        }

    _A113_PROTECTED:
        template< typename _T_cvt_ > bool _cvt_opt_arg( void* where_ ) {
            if constexpr( std::is_same_v< _T_cvt_, int32_t > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(int32_t*)where_ = (int32_t)strtol( endptr, &endptr, 10 );
                return endptr == &*_ctx->arg_tok->end();
            } else
            if constexpr( std::is_same_v< _T_cvt_, float > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(float*)where_ = (float)strtof( endptr, &endptr );
                return endptr == &*_ctx->arg_tok->end();
            } else
            if constexpr( std::is_same_v< _T_cvt_, double > ) {
                char* endptr = const_cast< char* >( _ctx->arg_tok->c_str() );
                *(double*)where_ = (double)strtod( endptr, &endptr );
                return endptr == &*_ctx->arg_tok->end();
            } 
            return false;
        }

        bool _cvt_opt_arg_single( Arg_ arg_ ) {
            const char* bad_cvt = nullptr;

            switch( arg_ ) {
                case Arg_text: {
                    _arg = (void*)_ctx->arg_tok;
                break; }
                case Arg_i32: {
                    _arg = (void*)_arg_mem;
                    A113_ASSERT_OR( _cvt_opt_arg< int32_t >( _arg ) ) bad_cvt = "i32";
                break; }
                case Arg_f32: {
                    _arg = (void*)_arg_mem;
                    A113_ASSERT_OR( _cvt_opt_arg< float >( _arg ) ) bad_cvt = "f32";
                break; }
                case Arg_f64: {
                    _arg = (void*)_arg_mem;
                    A113_ASSERT_OR( _cvt_opt_arg< double >( _arg ) ) bad_cvt = "f64";
                break; }
            }

            A113_ASSERT_OR( bad_cvt == nullptr ) {
                *_ctx->out += std::format( "Cannot convert \"{}\" to {} required by option \"{}\".\n", *_ctx->arg_tok, bad_cvt, *_ctx->opt_tok );
                return false;
            }
            return true;
        }

        void _cvt_opt_arg_multi_compact_init( Arg_ arg_ ) {
            _arg = (void*)_arg_mem;
            switch( arg_ ) {
                case Arg_text: { new (_arg) std::vector< const std::string* >{}; break; }
                case Arg_i32: { new (_arg) std::vector< int32_t >{}; break; }
                case Arg_f32: { new (_arg) std::vector< float >{}; break; }
                case Arg_f64: { new (_arg) std::vector< double >{}; break; }
            }
        }

        bool _cvt_opt_arg_multi_compact( Arg_ arg_ ) {
            switch( arg_ ) {
                case Arg_text: {
                    arg_textv().emplace_back( _ctx->arg_tok );
                break; }
                case Arg_i32: {
                    int32_t cvt = 0x0;
                    A113_ASSERT_OR( _cvt_opt_arg< int32_t >( &cvt ) ) return false;
                    arg_i32v().emplace_back( cvt );
                break; }
                case Arg_f32: {
                    float cvt = 0x0;
                    A113_ASSERT_OR( _cvt_opt_arg< float >( &cvt ) ) return false;
                    arg_f32v().emplace_back( cvt );
                break; }
                case Arg_f64: {
                    double cvt = 0x0;
                    A113_ASSERT_OR( _cvt_opt_arg< double >( &cvt ) ) return false;
                    arg_f64v().emplace_back( cvt );
                break; }
            }
            return true;
        }
    };

    using cmd_fnc_t = std::function< status_t( stencil_t& ) >;

    /**
     * @brief Command definition: name, options, callback, etc.
     */
    struct cmd_t {
        std::string            text   = {};
        std::vector< opt_t >   opts   = {};
        cmd_fnc_t              fnc    = nullptr;

        const opt_t* opt_by_short( char short_ ) const {
            auto itr = std::ranges::find_if( opts, [ short_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.sh0rt == short_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
        const opt_t* opt_by_long( const std::string& long_ ) const {
            auto itr = std::ranges::find_if( opts, [ long_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.l0ng == long_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
        const opt_t* opt_by_fast( int fid_ ) const {
            auto itr = std::ranges::find_if( opts, [ fid_ ] ( const opt_t& opt_ ) -> bool {
                return opt_.fast_id == fid_; 
            } );
            return itr != opts.end() ? &*itr : nullptr;
        }
    };

    using cmd_map_t = std::vector< cmd_t >;

public:
    Fastcli( void ) = default;

    Fastcli( const config_t& config_, const cmd_map_t& cmd_map_ ) 
    : _config{ config_ }, _cmd_map{ cmd_map_ }
    {}

_A113_PROTECTED:
    config_t            _config        = {};
    cmd_map_t           _cmd_map       = {};
    std::shared_mutex   _cmd_map_mtx   = {};

public:
    status_t bind( const config_t& config_, const cmd_map_t& cmd_map_ ) {
        _config  = config_;
        
        std::lock_guard lck{ _cmd_map_mtx };
        _cmd_map = cmd_map_;

        return A113_OK;
    }

_A113_PROTECTED:
    A113_inline bool _is_xtra_chr( const char c_ ) const {
        return _config.xtra_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    A113_inline bool _is_text_chr( const char c_ ) const {
        return std::isalpha( c_ ) || std::isdigit( c_ ) || _is_xtra_chr( c_ );
    }
    A113_inline bool _is_delim_chr( const char c_ ) const {
        return _config.delim_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    A113_inline bool _is_esc_chr( const char c_ ) const {
        return _config.esc_chrs.find( c_, 0x0 ) != std::string::npos;
    }
    A113_inline bool _is_qte_chr( const char c_ ) const {
        return _config.qte_chrs.find( c_, 0x0 ) != std::string::npos;
    }

_A113_PROTECTED:
    status_t _resolve_esc_chr( _parse_ctx_t* ctx_ ) {
        A113_ASSERT_OR( ++ctx_->id0 < ctx_->text.length() ) return A113_ERR_BADARG;

        static std::map< char, char > mrph_esc_chrs_map{
            { 'n', '\n' }, { 't', '\t' }
        };

        const char c = ctx_->text[ ctx_->id0 ];

        if( auto itr = mrph_esc_chrs_map.find( c ); itr != mrph_esc_chrs_map.end() ) {
            ctx_->crt_tok += itr->second;
        } else {
            ctx_->crt_tok += c;
        }
        return A113_OK;
    }

    status_t _split_cmd( _parse_ctx_t* ctx_ ) {        
        for(; ctx_->id0 < ctx_->text.length(); ++ctx_->id0 ) {
            const char c = ctx_->text[ ctx_->id0 ];

            if( _is_text_chr( c ) ) {
                ctx_->crt_tok += c; 
                continue;
            }

            if( _is_esc_chr( c ) ) {
                A113_ASSERT_OR( A113_OK == _resolve_esc_chr( ctx_ ) ) {
                    *ctx_->out += std::format( "Bad escape character near {}.\n", ctx_->id0 );
                    return A113_ERR_BADARG;
                }
                continue;
            }

            if( _is_delim_chr( c ) ) {
                if( not ctx_->in_qte ) {
                    ctx_->push_crt_tok();
                } else {
                    ctx_->crt_tok += c;
                }
                continue;
            }

            if( _is_qte_chr( c ) ) {
                ctx_->in_qte ^= true;
                continue;
            }

            *ctx_->out += std::format( "Bad character near {}.\n", ctx_->id0 );
            return A113_ERR_BADARG;
        }

        if( ctx_->in_qte ) {
            *ctx_->out += std::format( "Some quotes not closed properly.\n", ctx_->id0 );
            return A113_ERR_BADARG;
        }

        ctx_->push_crt_tok();
        return A113_OK;
    }

    status_t _consume_toks( _parse_ctx_t* ctx_ ) {
        auto itr = std::ranges::find_if( _cmd_map, [ ctx_ ] ( const cmd_t& cmd_ ) -> bool {
            return ctx_->toks[ 0x0 ] == cmd_.text; 
        } );
        A113_ASSERT_OR( itr != _cmd_map.end() ) {
            *ctx_->out += std::format( "Unknown command \"{}\".\n", ctx_->toks[ 0x0 ] );
            return A113_ERR_BADARG;
        }

        ctx_->cmd = &*itr;
        ctx_->id0 = 0x1;
        stencil_t stencil{
            ._ctx = ctx_
        };

        return itr->fnc( stencil );
    }

public:
    status_t execute( const std::string& text_, std::string* out_ ) {
        _parse_ctx_t ctx{
            .text = text_,
            .out  = out_
        };
        ctx.toks.reserve( 0x4 );

        status_t status = _split_cmd( &ctx );
        A113_ASSERT_OR( status == A113_OK ) return status;

        A113_ASSERT_OR( not ctx.toks.empty() ) {
            *out_ += "Empty command line."; return A113_ERR_BADARG;
        }

        std::shared_lock lck{ _cmd_map_mtx };
        return _consume_toks( &ctx );
    }

    A113_inline status_t operator () ( const std::string& text_, std::string* out_ ) {
        return execute( text_, out_ );
    }
};

inline char Fastcli::stencil_t::next( void ) {
#define _RET_DONE return 0x0;
#define _RET_ERR { _ctx->id0 = _ctx->toks.size(); return 0x1; }

    A113_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) _RET_DONE;

    const opt_t* opt     = nullptr;
    const auto&  opt_tok = _ctx->toks[ _ctx->id0 ]; 
    _ctx->opt_tok        = &opt_tok;
    
    if( opt_tok.starts_with( "--" ) ) {
        opt = _ctx->cmd->opt_by_long( opt_tok.substr( 0x2 ) );
    } else if( opt_tok[ 0x0 ] == '-' ) {
        opt = _ctx->cmd->opt_by_short( opt_tok[ 0x1 ] );
    } else {
        opt = _ctx->cmd->opt_by_fast( _fid++ );
        A113_ASSERT_OR( opt != nullptr ) {
            *_ctx->out += std::format( "Cannot resolve token \"{}\".\n", opt_tok );
            _RET_ERR;
        }
        goto l_fast_skip;
    }

    A113_ASSERT_OR( opt != nullptr ) {
        *_ctx->out += std::format( "Unknown option \"{}\".\n", opt_tok );
        _RET_ERR;
    }
    ++_ctx->id0;
    if( opt->arg != Arg_flag ) { A113_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) {
        *_ctx->out += std::format( "Missing argument(s) for option \"{}\".\n", opt_tok );
        _RET_ERR;
    } } else {
        return opt->sh0rt;
    }
   
l_fast_skip:
    _ctx->arg_tok = &_ctx->toks[ _ctx->id0++ ];

    switch( opt->argc ) {
        case Argc_single: {
            A113_ASSERT_OR( _cvt_opt_arg_single( opt->arg ) ) _RET_ERR;
        break; }

        case Argc_multi_compact: {
            _cvt_opt_arg_multi_compact_init( opt->arg );
            bool once = false;
            while( _cvt_opt_arg_multi_compact( opt->arg ) ) {
                once = true;
                A113_ASSERT_OR( _ctx->id0 < _ctx->toks.size() ) goto l_toks_consumed;
                _ctx->arg_tok = &_ctx->toks[ _ctx->id0++ ];
            }
            --_ctx->id0;
            
        l_toks_consumed:
            A113_ASSERT_OR( once ) {
                *_ctx->out += std::format( "Missing or bad argument(s) for option \"{}\".\n", opt->l0ng );
                _RET_ERR;
            }
        break; }
    }

    return opt->sh0rt;
#undef _RET_ERR
#undef _RET_DONE
}

}