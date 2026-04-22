#pragma once
/**
 * @file: osp/fastexp.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/gep/core.hpp>
#include <a113/gep/text_utils.hpp>

namespace a113::text {

template< typename _prec_t_ >
class Fastexp {
public:
    typedef   std::function< status_t( std::string_view, _prec_t_* ) >   defr_cb_t;

public:
    Fastexp( void ) = default;

    Fastexp( std::string_view exp_ ) {
        this->parse( exp_ );
    }

_A113_PROTECTED:
    struct _op_meta_t {
        uint8_t   prio   = 0x0;
        uint8_t   argc   = 0;
    };

    struct _sym_t {
        enum Typ_ {
            Typ_Unknwn, Typ_Num, Typ_Op, Typ_Paro, Typ_Parc, Typ_Defr
        };

        std::string   val   = {};
        Typ_          typ   = Typ_Unknwn;
        union {
            struct {
                _prec_t_   precvt;
            } num;
            struct { 
                _op_meta_t   meta;
                uint32_t     prehash; 
            } op;
        };
    };

_A113_PROTECTED:
    struct _parse_ctx_t {
        std::string_view        exp        = {};
        std::deque< _sym_t >*   rpn        = nullptr;
        int                     par        = 0;
        _sym_t::Typ_            prev_typ   = _sym_t::Typ_Unknwn;
    };

_A113_PROTECTED:
    std::deque< _sym_t >                              _rpn       = {};
    defr_cb_t                                         _defr      = {};
    std::unordered_map< unsigned char, _op_meta_t >   _opm_map   = {
        { '^', { 0x0C, 2 } },
        { '/', { 0x0B, 2 } },
        { '*', { 0x0B, 2 } },
        { '+', { 0x0A, 2 } },
        { '-', { 0x0A, 2 } }
    };

public:
    status_t bind( defr_cb_t defr_ ) {
        _defr = std::move( defr_ );
        return A113_OK;
    }

_A113_PROTECTED:
    bool _is_cvt_to_unary( const char c_, _parse_ctx_t* ctx_ ) const {
        if( c_ != '+' && c_ != '-' ) return false;
        return ( ctx_->prev_typ != _sym_t::Typ_Num && ctx_->prev_typ != _sym_t::Typ_Defr && ctx_->prev_typ != _sym_t::Typ_Parc ) 
               || 
               ctx_->prev_typ == _sym_t::Typ_Unknwn;
    }

    A113_inline bool _is_digit( const char c_ ) const {
        return c_ >= '0' && c_ <= '9';
    }

    A113_inline bool _is_alpha( const char c_ ) const {
        return ( c_ >= 'a' && c_ <= 'z' )
               ||
               ( c_ >= 'A' && c_ <= 'Z' );
    }

    A113_inline bool _is_word( const char c_ ) const {
        return this->_is_digit( c_ )
               ||
               this->_is_alpha( c_ )
               ||
               c_ == '_';
    }

    A113_inline bool _is_white( const char c_ ) const {
        return strchr( " \t", c_ ) != nullptr;
    }

_A113_PROTECTED:
    status_t _stack_rpn( _parse_ctx_t* ctx_ ) {
        std::deque< _sym_t >   holding    = {};
        
        /// === utility === ///
        auto push_holding = [ &holding, ctx_ ] ( _sym_t&& sym_ ) -> void {
            holding.emplace_front( std::move( sym_ ) );
        };
        auto push_rpn = [ ctx_ ] ( _sym_t&& sym_ ) -> void {
            ctx_->rpn->emplace_back( std::move( sym_ ) );
        };
        auto drain_prio = [ &holding, &push_rpn] ( const unsigned char pr_ ) -> void {
            while( not holding.empty() && holding.front().typ == _sym_t::Typ_Op ) {
                auto& top = holding.front();
                
                if( top.op.meta.prio >= pr_ ) {
                    push_rpn( std::move( top ) ); holding.pop_front();
                } else {
                    break;
                }
            }
        };

        for( int idx = 0x0; idx < ctx_->exp.length(); ++idx ) { 
            const char c = ctx_->exp[ idx ];

            /// === trim === ///
            if( _is_white( c ) ) continue;

            /// === numeric === ///
            if( this->_is_digit( c ) ) {
                bool dot = false;

                const char* begin = ctx_->exp.data() + idx;
                const char* end   = std::find_if( begin, ctx_->exp.data() + ctx_->exp.length(), [ this, &dot ] ( const char c_ ) -> bool {
                    if( c_ == '.' ) return std::exchange( dot, true );
                    return not this->_is_digit( c_ );
                } );
                const size_t diff = end - begin;

                _prec_t_ precvt = 0.0;
                auto [ ptr, ec ] = std::from_chars( begin, end, precvt );
                if( ptr != end || ec != std::errc{ 0x0 } ) return A113_ERR_BADARG;

                push_rpn( { 
                    .val = { begin, diff }, 
                    .typ = _sym_t::Typ_Num, 
                    .num = {
                        .precvt = precvt 
                    }
                } );
                ctx_->prev_typ = _sym_t::Typ_Num;

                idx += diff - 1;
                goto l_type_asserted;
            } 

            /// === text === ///
            if( this->_is_alpha( c ) ) {
                const char* begin = ctx_->exp.data() + idx;
                const char* end   = std::find_if( begin, ctx_->exp.data() + ctx_->exp.length(), [ this ] ( const char c_ ) -> bool {
                    return not this->_is_word( c_ );
                } );
                const size_t diff = end - begin;
                
                std::string str{ begin, diff };

                if( *end == '(' ) {
                    _op_meta_t next_opm = {
                        .prio = 0xFE,
                        .argc  = 1
                    };
                    drain_prio( next_opm.prio );
                    const auto prehash = text::hash( str );
                    push_holding( { 
                        .val = std::move( str ), 
                        .typ = _sym_t::Typ_Op, 
                        .op  = {
                            .meta    = std::move( next_opm ),
                            .prehash = prehash
                        } 
                    } );
                    ctx_->prev_typ = _sym_t::Typ_Op;
                } else {
                    push_rpn( { .val = std::move( str ), .typ = _sym_t::Typ_Defr } );
                    ctx_->prev_typ = _sym_t::Typ_Defr;
                }

                idx += diff - 1;
                goto l_type_asserted;
            }
            
            /// === paranthesis === ///
            if( c == '(' ) {
                push_holding( { .val = { '(' }, .typ = _sym_t::Typ_Paro } );
                ++ctx_->par;
                ctx_->prev_typ = _sym_t::Typ_Paro;
                goto l_type_asserted;
            }
            if( c == ')' ) {
                A113_ASSERT_OR( ctx_->par > 0 ) return A113_ERR_BADARG;

                while( not holding.empty() ) {
                    auto& top = holding.front();

                    if( top.typ == _sym_t::Typ_Paro ) { holding.pop_front(); break; } 

                    push_rpn( std::move( top ) ); holding.pop_front();
                }
                --ctx_->par;
                ctx_->prev_typ = _sym_t::Typ_Parc;
                goto l_type_asserted;
            }

            /// === basic operator === ///
            if( auto itr_next_opm = _opm_map.find( c ); itr_next_opm != _opm_map.end() ) {
                _op_meta_t next_opm = itr_next_opm->second;

                if( _is_cvt_to_unary( c, ctx_ ) ) {
                    next_opm.prio = 0xFF;
                    next_opm.argc  = 1;
                }

                drain_prio( next_opm.prio );
                push_holding( { 
                    .val = { c }, 
                    .typ = _sym_t::Typ_Op, 
                    .op  = {
                        .meta    = std::move( next_opm ),
                        .prehash = text::hash( { c } )
                    }
                } );
                ctx_->prev_typ = _sym_t::Typ_Op;
                goto l_type_asserted;
            }

            return A113_ERR_BADARG;

        l_type_asserted:
            continue;
        }

        A113_ASSERT_OR( ctx_->par == 0 ) return A113_ERR_BADARG;

        if( not holding.empty() ) ctx_->rpn->insert_range( ctx_->rpn->end(), std::move( holding ) );
        return A113_OK;
    }

public:
    status_t parse( std::string_view exp_ ) {
        status_t status = A113_OK;

        _rpn.clear();
        _parse_ctx_t ctx = {
            .exp = exp_,
            .rpn = &_rpn
        };

        status = this->_stack_rpn( &ctx ); 
        A113_ASSERT_OR( A113_OK == status ) return status;
        return A113_OK;
    }

    status_t resolve( _prec_t_* result_ ) {
        _prec_t_ rslvstk[ _rpn.size() ];
        int      rslvtop   = -0x1;

        for( const auto& sym : _rpn ) {
            switch( sym.typ ) {
                case _sym_t::Typ_Num: {
                    rslvstk[ ++rslvtop ] = sym.num.precvt;
                break; }

                case _sym_t::Typ_Defr: {
                    _prec_t_ res;
                    A113_ASSERT_OR( A113_OK == this->_defr( sym.val, &res ) ) return A113_ERR_USERCALL;

                    rslvstk[ ++rslvtop ] = res;
                break; }
                
                case _sym_t::Typ_Op: {
                    _prec_t_* regs = rslvstk + rslvtop;
                    _prec_t_  clps = _prec_t_{ 0x0 };

                    rslvtop -= sym.op.meta.argc;

                    switch( sym.op.meta.argc ) {
                        case 1: {
                            switch( sym.op.prehash ) {
                                case hash( "+" ):   clps = regs[0];             break;
                                case hash( "-" ):   clps = -regs[0];            break;
                                case hash( "sin" ): clps = std::sin( regs[0] ); break;
                                case hash( "cos" ): clps = std::cos( regs[0] ); break;
                                case hash( "tan" ): clps = std::tan( regs[0] ); break;
                            }
                        break; }

                        case 2: {
                            switch( sym.op.prehash ) {
                            case hash( "^" ): clps = std::pow( regs[-1], regs[0] ); break;
                            case hash( "/" ): clps = regs[-1] / regs[0];            break;
                            case hash( "*" ): clps = regs[-1] * regs[0];            break;
                            case hash( "+" ): clps = regs[-1] + regs[0];            break;
                            case hash( "-" ): clps = regs[-1] - regs[0];            break;
                        }
                        break; }
                    }
                    rslvstk[ ++rslvtop ] = clps;
                break; }
            }
        }
        *result_ = rslvstk[ rslvtop ];
        return A113_OK;
    }

};

}