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

template< typename _pcsn_t_ >
class Fastexp {
public:
    typedef   std::function< status_t( std::string_view, _pcsn_t_* ) >   defr_cb_t;

public:
    Fastexp( void ) = default;

    Fastexp( std::string_view exp_ ) {
        this->parse( exp_ );
    }

_A113_PROTECTED:
    struct _op_t {
        uint8_t   precedence   = 0x0;
        uint8_t   arg_count    = 0;
    };

    struct _sym_t {
        enum Typ_ {
            Typ_Unknwn, Typ_Num, Typ_Op, Typ_Paro, Typ_Parc, Typ_Defr
        };

        std::string   val   = {};
        Typ_          typ   = Typ_Unknwn;
        _op_t         op    = {};
    };

_A113_PROTECTED:
    struct _parse_ctx_t {
        std::string_view        exp        = {};
        std::deque< _sym_t >*   rpn        = nullptr;
        int                     par        = 0;
        _sym_t::Typ_            prev_typ   = _sym_t::Typ_Unknwn;
    };

_A113_PROTECTED:
    std::deque< _sym_t >                         _rpn      = {};
    defr_cb_t                                    _defr     = {};
    std::unordered_map< unsigned char, _op_t >   _op_map   = {
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
        
        auto push_holding = [ &holding, ctx_ ] ( _sym_t&& sym_ ) -> void {
            holding.emplace_front( std::move( sym_ ) );
        };
        auto push_rpn = [ ctx_ ] ( _sym_t&& sym_ ) -> void {
            ctx_->rpn->emplace_back( std::move( sym_ ) );
        };
        auto drain_precedence = [ &holding, &push_rpn] ( const unsigned char pr_ ) -> void {
            while( not holding.empty() && holding.front().typ == _sym_t::Typ_Op ) {
                auto& top = holding.front();
                
                if( top.op.precedence >= pr_ ) {
                    push_rpn( std::move( top ) ); holding.pop_front();
                } else {
                    break;
                }
            }
        };

        for( int idx = 0x0; idx < ctx_->exp.length(); ++idx ) { 
            const char c = ctx_->exp[ idx ];

            if( _is_white( c ) ) continue;

            if( this->_is_digit( c ) ) {
                bool dot = false;

                const char* begin = ctx_->exp.data() + idx;
                const char* end   = std::find_if( begin, ctx_->exp.data() + ctx_->exp.length(), [ this, &dot ] ( const char c_ ) -> bool {
                    if( c_ == '.' ) return std::exchange( dot, true );
                    return not this->_is_digit( c_ );
                } );
                const size_t diff = end - begin;

                push_rpn( { .val = { begin, diff }, .typ = _sym_t::Typ_Num } );
                ctx_->prev_typ = _sym_t::Typ_Num;

                idx += diff - 1;
                goto l_type_asserted;
            } 

            if( this->_is_alpha( c ) ) {
                const char* begin = ctx_->exp.data() + idx;
                const char* end   = std::find_if( begin, ctx_->exp.data() + ctx_->exp.length(), [ this ] ( const char c_ ) -> bool {
                    return not this->_is_word( c_ );
                } );
                const size_t diff = end - begin;
                
                std::string str{ begin, diff };

                if( *end == '(' ) {
                    _op_t next_op = {
                        .precedence = 0xFE,
                        .arg_count  = 1
                    };
                    drain_precedence( next_op.precedence );
                    push_holding( { .val = std::move( str ), .typ = _sym_t::Typ_Op, .op = std::move( next_op ) } );
                    ctx_->prev_typ = _sym_t::Typ_Op;
                } else {
                    push_rpn( { .val = std::move( str ), .typ = _sym_t::Typ_Defr } );
                    ctx_->prev_typ = _sym_t::Typ_Defr;
                }

                idx += diff - 1;
                goto l_type_asserted;
            }

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

            if( auto itr_next_op = _op_map.find( c ); itr_next_op != _op_map.end() ) {
                _op_t next_op = itr_next_op->second;

                if( _is_cvt_to_unary( c, ctx_ ) ) {
                    next_op.precedence = 0xFF;
                    next_op.arg_count  = 1;
                }

                drain_precedence( next_op.precedence );
                push_holding( { .val = { c }, .typ = _sym_t::Typ_Op, .op = std::move( next_op ) } );
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

    status_t resolve( _pcsn_t_* result_ ) {
        std::deque< _pcsn_t_ > resolver;

        for( const auto& sym : _rpn ) {
            switch( sym.typ ) {
                case _sym_t::Typ_Num: {
                    _pcsn_t_ cvt;
                    const char*   begin = sym.val.data();
                    const char*   end   = sym.val.data() + sym.val.length();

                    auto [ ptr, ec ] = std::from_chars( begin, end, cvt );
                    if( ptr != end || ec != std::errc{ 0x0 } ) return A113_ERR_BADARG;

                    resolver.push_front( cvt );
                break; }

                case _sym_t::Typ_Defr: {
                    _pcsn_t_ res;
                    A113_ASSERT_OR( A113_OK == this->_defr( sym.val, &res ) ) return A113_ERR_USERCALL;

                    resolver.push_front( res );
                break; }
                
                case _sym_t::Typ_Op: {
                    _pcsn_t_ regs[ sym.op.arg_count ];

                    for( uint8_t idx = 0x0; idx < sym.op.arg_count; ++idx ) {
                        A113_ASSERT_OR( not resolver.empty() ) return A113_ERR_BADARG;
                        regs[ idx ] = resolver.front(); resolver.pop_front();
                    }

                    _pcsn_t_ collapsed = _pcsn_t_{ 0x0 };
                    switch( sym.op.arg_count ) {
                        case 1: {
                            switch( hash( sym.val ) ) {
                                case hash( "+" ): collapsed = regs[ 0x0 ]; break;
                                case hash( "-" ): collapsed = -regs[ 0x0 ]; break;
                                case hash( "sin" ): collapsed = std::sin( regs[ 0x0 ] ); break;
                                case hash( "cos" ): collapsed = std::cos( regs[ 0x0 ] ); break;
                                case hash( "tan" ): collapsed = std::tan( regs[ 0x0 ] ); break;
                            }
                        break; }

                        case 2: {
                            switch( hash( sym.val ) ) {
                            case hash( "^" ): collapsed = std::pow( regs[ 0x1 ], regs[ 0x0 ] ); break;
                            case hash( "/" ): collapsed = regs[ 0x1 ] / regs[ 0x0 ]; break;
                            case hash( "*" ): collapsed = regs[ 0x1 ] * regs[ 0x0 ]; break;
                            case hash( "+" ): collapsed = regs[ 0x1 ] + regs[ 0x0 ]; break;
                            case hash( "-" ): collapsed = regs[ 0x1 ] - regs[ 0x0 ]; break;
                        }
                        break; }
                    }
                    resolver.push_front( collapsed );
                break; }
            }
        }
        *result_ = resolver.front();
        return A113_OK;
    }

};

}