#pragma once
/**
 * @file: osp/fastcli.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>

namespace a113::text {

class Fastcli {
public:
    struct cmd_t;

_A113_PROTECTED:
    /**
     * @brief Used across internal functions while parsing a command.
     */
    struct _parse_ctx_t {
        const std::string&           text      = {};
        std::string*                 out       = nullptr;
        std::vector< std::string >   toks      = {};
        const cmd_t*                 cmd       = nullptr;
        int                          cid       = 0x0;
        std::string                  crt_tok   = "";
        bool                         in_qte    = false;

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
        std::string   xtra_chrs   = "._/";
        std::string   esc_chrs    = "\\";
        std::string   qte_chrs    = "\"\'";
        std::string   var_chrs    = "$";
    };
    
    /**
     * @brief Command option.
     * @details (1): --some-opt (2): --some-opt-w-arg 69 (3): -s (4): -S 69
     *          An option that does not start with neither '-' or '--', is considered the nth option (using fast index).
     */
    struct opt_t {
        enum Type_ {
            Type_none,
            Type_text,
            Type_int, Type_uint, Type_float
        };

        char          sh0rt     = '\0';
        std::string   l0ng      = {};
        int           fast_id   = -0x1;
        Type_         type      = Type_none;
    };

    /**
     * @brief Structure passed to the user parse callback.
     * @details Usage is intended to be as close as possible to getopt_long().
     */
    struct tool_t {
        _parse_ctx_t*   _ctx   = nullptr;
        int             _fid   = 0x0;

        std::tuple< char, void* > get( void );
    };

    using cmd_fnc_t = std::function< status_t( tool_t& ) >;

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
        A113_ASSERT_OR( ++ctx_->cid < ctx_->text.length() ) return false;

        static std::map< char, char > mrph_esc_chrs_map{
            { 'n', '\n' }, { 't', '\t' }
        };

        const char c = ctx_->text[ ctx_->cid ];

        if( auto itr = mrph_esc_chrs_map.find( c ); itr != mrph_esc_chrs_map.end() ) {
            ctx_->crt_tok += itr->second;
        } else {
            ctx_->crt_tok += c;
        }
        return A113_OK;
    }

    status_t _split_cmd( _parse_ctx_t* ctx_ ) {        
        for(; ctx_->cid < ctx_->text.length(); ++ctx_->cid ) {
            const char c = ctx_->text[ ctx_->cid ];

            if( _is_text_chr( c ) ) {
                ctx_->crt_tok += c; 
                continue;
            }

            if( _is_esc_chr( c ) ) {
                A113_ASSERT_OR( A113_OK == _resolve_esc_chr( ctx_ ) ) {
                    *ctx_->out += std::format( "Bad escape character near {}.\n", ctx_->cid );
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

            *ctx_->out += std::format( "Bad character near {}.\n", ctx_->cid );
            return A113_ERR_BADARG;
        }

        if( ctx_->in_qte ) {
            *ctx_->out += std::format( "Some quotes not closed properly.\n", ctx_->cid );
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
            *ctx_->out += std::format( "Unknown command \"{}\".", ctx_->toks[ 0x0 ] );
            return A113_ERR_BADARG;
        }

        ctx_->cmd = &*itr;
        ctx_->cid = 0x1;
        tool_t tool{
            ._ctx = ctx_
        };

        A113_ASSERT_OR( itr->fnc( tool ) == A113_OK ) {
            *ctx_->out += "Failed to execute command."; 
            return A113_ERR_USERCALL;
        }

        return A113_OK;
    }

public:
    status_t parse( const std::string& cmd_, std::string* out_ ) {
        _parse_ctx_t ctx{
            cmd: cmd_,
            out: out_
        };
        ctx.toks.reserve( 0x4 );

        status_t status = _split_cmd( &ctx );
        A113_ASSERT_OR( status == A113_OK ) return status;

        A113_ASSERT_OR( not ctx.toks.empty() ) {
            *out_ += "Empty command line."; return A113_ERR_BADARG;
        }

        std::shared_lock lck{ _cmd_map_mtx };
        _consume_toks( &ctx );

        return A113_OK;
    }

};

std::tuple< char, void* > Fastcli::tool_t::get( void ) {
#define _RET_DONE return { 0x0, nullptr }
#define _RET_ERR return { 0x1, nullptr }

    const opt_t* opt = nullptr;

    A113_ASSERT_OR( _ctx->cid < _ctx->toks.size() ) _RET_DONE;

    const auto& crt_tok = _ctx->toks[ _ctx->cid ];
    if( crt_tok.starts_with( "--" ) ) {
        opt = _ctx->cmd->opt_by_long( crt_tok.substr( 0x2 ) );
    } else if( crt_tok[ 0x0 ] == '-' ) {
        opt = _ctx->cmd->opt_by_short( crt_tok[ 0x1 ] );
    } else {
        opt = _ctx->cmd->opt_by_fast( _fid++ );
        A113_ASSERT_OR( opt == nullptr ) {
            *_ctx->out += std::format( "Cannot resolve token \"{}\".", crt_tok );
            _RET_ERR;
        }
    }

    A113_ASSERT_OR( opt == nullptr ) {
        *_ctx->out += std::format( "Unknown option \"{}\".", crt_tok );
        _RET_ERR;
    }
    ++_ctx->cid;
    if( opt->type != opt_t::Type_none ) A113_ASSERT_OR( _ctx->cid < _ctx->toks.size() ) {
        *_ctx->out += std::format( "Missing argument for option \"{}\".", crt_tok );
        _RET_ERR;
    }

    const auto& arg = _ctx->toks[ ]

    ++_ctx->cid;
}

}