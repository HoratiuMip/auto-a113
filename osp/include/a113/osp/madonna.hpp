#pragma once
/**
 * @file: osp/madonna.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
/* === engine includes === */
#include <a113/osp/core.hpp>
/* === stl includes  */
#include <cmath>
#include <complex>
/* === excom includes === */
#include <lapacke.h>

/* === internal bridge === */
namespace a113::_mdn {

#ifdef A113_MDN_REAL_T
    typedef   A113_MDN_REAL_T   real_t;
#else
    typedef   float             real_t;
#endif

template< size_t _N_, typename _prec_t_ = real_t > struct arr_t : std::array< _prec_t_, _N_ > {
public:
    A113_inline _prec_t_* get( void ) { return this->data(); }
    A113_inline const _prec_t_* get( void ) const { return this->data(); }

    A113_inline operator _prec_t_* ( void ) { return this->get; } 
    A113_inline operator const _prec_t_* ( void ) const { return this->get(); }

public:
    _prec_t_ norm( void ) const {
        _prec_t_ res = 0;
        for( size_t c = 0x0; c < _N_; ++c ) res += std::pow( (*this)[c], 2 );
        return std::sqrt( res );
    }

public:
    _prec_t_ dot( const auto& rhs_ ) const {
        _prec_t_ res = 0;
        for( size_t c = 0x0; c < _N_ && c < rhs_.size(); ++c ) res += (*this)[c] * rhs_[c];
        return res;
    }

public:
#define _MDN_ARR_OP_(o) \
    template< typename _T_ > \
    arr_t< _N_, _prec_t_ > operator o ( const _T_& rhs_ ) const requires std::is_arithmetic_v< _T_ > { \
        arr_t< _N_, _prec_t_ > res; \
        for( size_t c = 0x0; c < _N_; ++c ) { \
            res[ c ] = (*this)[c] o rhs_; \
        } \
        return res; \
    }
    _MDN_ARR_OP_(+)
    _MDN_ARR_OP_(-)
    _MDN_ARR_OP_(*)
    _MDN_ARR_OP_(/)
#undef _MDN_ARR_OP_
#define _MDN_ARR_OP_(o) \
    arr_t< _N_, _prec_t_ > operator o ( const auto& rhs_ ) const { \
        arr_t< _N_, _prec_t_ > res; \
        for( size_t c = 0x0; c < _N_ && c < rhs_.size(); ++c ) { \
            res[ c ] = (*this)[c] o rhs_[c]; \
        } \
        return res; \
    }
    _MDN_ARR_OP_(+)
    _MDN_ARR_OP_(-)
    _MDN_ARR_OP_(*)
    _MDN_ARR_OP_(/)
#undef _MDN_ARR_OP_
#define _MDN_ARR_OPE_(oe) \
    template< typename _T_ > \
    arr_t< _N_, _prec_t_ > & operator oe ( const _T_& rhs_ ) requires std::is_arithmetic_v< _T_ > { \
        for( size_t c = 0x0; c < _N_; ++c ) { \
            (*this)[c] oe rhs_; \
        } \
        return *this; \
    } 
    _MDN_ARR_OPE_(+=)
    _MDN_ARR_OPE_(-=)
    _MDN_ARR_OPE_(*=)
    _MDN_ARR_OPE_(/=)
#undef _MDN_ARR_OPE_
#define _MDN_ARR_OPE_(oe) \
    arr_t< _N_, _prec_t_ > & operator oe ( const auto& rhs_ ) { \
        for( size_t c = 0x0; c < _N_ && c < rhs_.size(); ++c ) { \
            (*this)[c] oe rhs_[c]; \
        } \
        return *this; \
    } 
    _MDN_ARR_OPE_(+=)
    _MDN_ARR_OPE_(-=)
    _MDN_ARR_OPE_(*=)
    _MDN_ARR_OPE_(/=)
#undef _MDN_ARR_OPE_

    arr_t< _N_, _prec_t_ > operator - ( void ) const {
        return { -(*this)[0x0], -(*this)[0x1] };
    }

public:
    _prec_t_ x( void ) const requires( _N_ >= 1 ) { return (*this)[0x0]; }
    _prec_t_ y( void ) const requires( _N_ >= 2 ) { return (*this)[0x1]; }
    _prec_t_ z( void ) const requires( _N_ >= 3 ) { return (*this)[0x2]; }

};

class _bridge_t {
public:
    _bridge_t( void ) {
        logger = spdlog::stdout_color_mt( A113_VERSION_STRING"/madonna" ); 
        logger->set_pattern( A113_SPDLOG_PATTERN );

        logger->info( "bridge: init OK." );
    }

_A113_PROTECTED:
    HVec< spdlog::logger >   logger   = nullptr;

public:
    A113_inline spdlog::logger* operator -> ( void ) { return logger.get(); }

}; inline _bridge_t BridgE;

};

/* === working structures  === */
namespace a113::mdn_0 { using namespace _mdn;

enum DiscDiffMth_ {
    DiscDiffMth_FwdEuler,
    DiscDiffMth_Tustin
};

template< typename _prec_t_ = real_t > struct vec2_t {
    _prec_t_   x;
    _prec_t_   y;
};

template< typename _prec_t_ = real_t > using fnc_1d_t = std::function< _prec_t_( _prec_t_ ) >;
template< typename _prec_t_ = real_t > using fnc_2d_t = std::function< _prec_t_( _prec_t_, _prec_t_ ) >;
#define MDN_FNC_2D_L( _prec_t_ ) ( _prec_t_ x1, _prec_t_ x2 ) -> _prec_t_

template< typename _prec_t_ = real_t > struct tfc_t {
    int        n     = 0;
    int        m     = 0;
    _prec_t_*   den   = nullptr;
    _prec_t_*   num   = nullptr;
};


template< typename _prec_t_ = real_t >
_prec_t_ linspace_n( _prec_t_* v_, int n_, _prec_t_ low_, _prec_t_ upp_ ) {
    _prec_t_ step = ( upp_ - low_ ) / n_;
    for( int n = 0; n < n_; ++n ) {
        v_[ n ] = low_;
        low_ += step;
    } 
    return step;
}
 
template< typename _prec_t_ = real_t, typename _Op_ >
_prec_t_ roam_acc_2( _prec_t_* v1_, _prec_t_* v2_, int n_, _prec_t_ acc_, _Op_ op_ ) {
    for( int i = 0; i < n_; ++i ) acc_ += op_( v1_[i], v2_[i] );
    return acc_;
}


template< typename _prec_t_ = real_t >
status_t roots( const _prec_t_* co_, int n_, _prec_t_* rr_, _prec_t_* ri_ ) {
    _prec_t_ A[ n_ ][ n_ ]; memset( A, 0x0, n_*n_*sizeof( _prec_t_ ) );

    for( int i = 0x0; i < n_; ++i ) {
        A[ 0x0 ][ i ] = -co_[ i+1 ] / co_[ 0x0 ];
        if( i != n_-1 ) A[ i+1 ][ i ] = _prec_t_{1};
    }

    int info;   
    if constexpr( std::is_same_v< float, _prec_t_ > ) {
        info = LAPACKE_sgeev( LAPACK_ROW_MAJOR, 'N', 'N', n_, (float*)A, n_, rr_, ri_, nullptr, 1, nullptr, 1 );
    } else if constexpr( std::is_same_v< double, _prec_t_ > ) {
        info = LAPACKE_dgeev( LAPACK_ROW_MAJOR, 'N', 'N', n_, (double*)A, n_, rr_, ri_, nullptr, 1, nullptr, 1 );
    }
    A113_ASSERT_OR( info == 0x0 ) return A113_ERR_EXCOMCALL;
    return A113_OK;
}   


template< typename _prec_t_ = real_t >
status_t invm( _prec_t_* m_, int n_ ) {
    int info = 0x0;
    int ipiv[ n_ ];

    if constexpr( std::is_same_v< float, _prec_t_ > ) {
        info = LAPACKE_sgetrf( LAPACK_ROW_MAJOR, n_, n_, m_, n_, ipiv );
    } else if constexpr( std::is_same_v< double, _prec_t_ > ) {
        info = LAPACKE_dgetrf( LAPACK_ROW_MAJOR, n_, n_, m_, n_, ipiv );
    }
    A113_ASSERT_OR( info == 0x0 ) return A113_ERR_EXCOMCALL;

    if constexpr( std::is_same_v< float, _prec_t_ > ) {
        info = LAPACKE_sgetri( LAPACK_ROW_MAJOR, n_, m_, n_, ipiv );
    } else if constexpr( std::is_same_v< double, _prec_t_ > ) {
        info = LAPACKE_dgetri( LAPACK_ROW_MAJOR, n_, m_, n_, ipiv );
    }
    A113_ASSERT_OR( info == 0x0 ) {
        BridgE->error( "invm: could not invert matrix." );
        return A113_ERR_FLOW;
    }

    return A113_OK;
}


template< typename _prec_t_ = real_t >
_prec_t_ tfc_step_fwdeul( _prec_t_* den_, int n_, _prec_t_* num_, int m_, _prec_t_* dy_h_, _prec_t_* du_h_, _prec_t_ u_, _prec_t_ t_ ) {
    _prec_t_ dny = num_[ m_-1 ] * u_;

    du_h_[ 0x1 ] = du_h_[ 0x0 ];
    du_h_[ 0x0 ] = u_;

    int ui = 0x0;
    for( int ord = 1; ord < m_; ++ord ) {
        const int i       = ord<<1;
        const _prec_t_ d = ( du_h_[ ui ] - du_h_[ ui+1 ] );

        du_h_[ i+1 ] = du_h_[ i ];
        du_h_[ i ]   = d;

        dny += num_[ m_-ord-1 ] * d/t_;

        ui = i;
    }

    for( int ord = 0; ord < n_-1; ++ord ) {
        const int i = ord<<1;

        dny -= dy_h_[ i ] * den_[ n_-ord-1 ];
    }
    
    int li = (n_-1) << 1;

    dy_h_[ li+1 ] = dy_h_[ li ];
    dy_h_[ li ]   = dny / den_[ 0x0 ];

    for( int ord = n_-2; ord >= 0; --ord ) {
        const int i       = ord<<1;
        const _prec_t_ d = (dy_h_[ li ] + dy_h_[ li+1 ]) / 2;

        dy_h_[ i+1 ] = dy_h_[ i ];
        dy_h_[ i ]   += d*t_;

        li = i;
    }

    return dy_h_[ 0x0 ];
}

template< typename _prec_t_ = real_t >
_prec_t_ d1_f1( _prec_t_ x_, _prec_t_ r_, fnc_1d_t< _prec_t_ > fnc_ ) {
    const _prec_t_ lb = fnc_( x_ - r_ );
    const _prec_t_ rb = fnc_( x_ + r_ );
    return ( rb-lb ) / ( r_*2 ); 
} 

template< typename _prec_t_ = real_t >
_prec_t_ d2_f1( _prec_t_ x_, _prec_t_ r_, fnc_1d_t< _prec_t_ > fnc_ ) {
    const _prec_t_ lb = d1_f1( x_ - r_, r_, fnc_ );
    const _prec_t_ rb = d1_f1( x_ + r_, r_, fnc_ );
    return ( rb-lb ) / ( r_*2 ); 
}

template< typename _prec_t_ = real_t >
auto d1_f2( _prec_t_ x_, _prec_t_ y_, _prec_t_ r_, fnc_2d_t< _prec_t_ > fnc_ ) {
    arr_t< 2, _prec_t_ > d;

    const auto fx = [ &fnc_, &y_ ] ( _prec_t_ x_ ) -> _prec_t_ { return fnc_( x_, y_ ); };
    const auto fy = [ &fnc_, &x_ ] ( _prec_t_ y_ ) -> _prec_t_ { return fnc_( x_, y_ ); };

    d[ 0x0 ] = d1_f1< _prec_t_ >( x_, r_, fx );
    d[ 0x1 ] = d1_f1< _prec_t_ >( y_, r_, fy );
    return d;
}

template< typename _prec_t_ = real_t >
auto d2_f2( _prec_t_ x_, _prec_t_ y_, _prec_t_ r_, fnc_2d_t< _prec_t_ > fnc_ ) {
    arr_t< 2, _prec_t_ > d2;

    const auto fx = [ &fnc_, &y_ ] ( _prec_t_ x_ ) -> _prec_t_ { return fnc_( x_, y_ ); };
    const auto fy = [ &fnc_, &x_ ] ( _prec_t_ y_ ) -> _prec_t_ { return fnc_( x_, y_ ); };

    d2[ 0x0 ] = d2_f1< _prec_t_ >( x_, r_, fx );
    d2[ 0x1 ] = d2_f1< _prec_t_ >( y_, r_, fy );
    return d2;
}

template< typename _prec_t_ = real_t >
auto d2h_f2( _prec_t_ x_, _prec_t_ y_, _prec_t_ r_, fnc_2d_t< _prec_t_ > fnc_ ) {
    arr_t< 4, _prec_t_ > d2h;

    const auto d2 = d2_f2< _prec_t_ >( x_, y_, r_, fnc_ );
    d2h[ 0x0 ] = d2[ 0x0 ];
    d2h[ 0x3 ] = d2[ 0x1 ];

    fnc_1d_t< _prec_t_ > fdxy = [ &fnc_, &x_, &r_ ]( _prec_t_ y_ ) -> _prec_t_ { return d1_f2( x_, y_, r_, fnc_ )[ 0x0 ]; };
    fnc_1d_t< _prec_t_ > fdyx = [ &fnc_, &y_, &r_ ]( _prec_t_ x_ ) -> _prec_t_ { return d1_f2( x_, y_, r_, fnc_ )[ 0x1 ]; };

    d2h[ 0x1 ] = d1_f1( y_, r_, fdxy );
    d2h[ 0x2 ] = d1_f1( x_, r_, fdyx );

    return d2h;
}


template< typename _prec_t_ = real_t >
vec2_t< _prec_t_ > search_mf1_elimgr( _prec_t_ a_, _prec_t_ b_, _prec_t_ eps_, int* n_, fnc_1d_t< _prec_t_ > fnc_ ) {
    if( b_ < a_ ) std::swap( b_, a_ );
    auto d = b_ - a_;

    int n = 0;
    while( b_ - a_ >= eps_ ) { ++n;
        d *= 0.618;

        const auto x1 = b_ - d;
        const auto x2 = a_ + d;

        if( fnc_( x1 ) <= fnc_( x2 ) )
            b_ = x2;
        else
            a_ = x1;
    } 

    if( n_ ) *n_ = n;

    const auto x = (a_ + b_) / 2;
    return { .x = x, .y = fnc_( x ) };
}

template< typename _prec_t_ = real_t >
vec2_t< _prec_t_ > search_mf1_elimfib( _prec_t_ a_, _prec_t_ b_, _prec_t_ eps_, int* n_, fnc_1d_t< _prec_t_ > fnc_ ) {
    if( b_ < a_ ) std::swap( b_, a_ );

    auto f1 = _prec_t_{3};
    auto f2 = _prec_t_{2}; 

    int n = 0;
    while( b_ - a_ >= eps_ ) { ++n;
        const auto d  = (b_ - a_) * f2/f1;
        const auto x1 = b_ - d;
        const auto x2 = a_ + d;

        if( fnc_( x1 ) <= fnc_( x2 ) )
            b_ = x2;
        else
            a_ = x1;

        f1 += std::exchange( f2, f1 );
    }

    if( n_ ) *n_ = n;
    
    const auto x = (a_ + b_) / 2;
    return { .x = x, .y = fnc_( x ) };
}

}

/* === memory structures === */
namespace a113::mdn_1 { using namespace _mdn;

template< typename _prec_t_ = real_t > using vec = std::vector< _prec_t_ >;

template< typename _prec_t_ = real_t > struct vec_t {
_A113_PROTECTED:
    _prec_t_*   _cmps   = nullptr;

public:
    union {
        struct {
            _prec_t_   x;
            _prec_t_   y;
            _prec_t_   z;
        };
        struct {
            int   _sz;
        };
    };

public:
    A113_inline _prec_t_* cmps( void ) {
        return _cmps ? _cmps : &x;
    }

};

template< typename _prec_t_ = real_t > struct rvec {
public: 
    rvec( void ) = default;

    rvec( int cap_ ) { this->bind( cap_ ); }

_A113_PROTECTED:
    std::unique_ptr< _prec_t_[] >   _data   = nullptr;
    int                              _head   = 0x0;
    int                              _sz     = 0;
    int                              _cap    = 0;

public:
    status_t bind( int cap_ ) {
        _data.reset( new _prec_t_[ _cap = cap_ ] );
        _head = 0; _sz = 0;

        return A113_OK;
    }

public:
    _prec_t_* data( void ) { return _data.get(); }
    int head( void ) { return _head; }
    int size( void ) { return _sz; }

public:
    _prec_t_& push_back( const _prec_t_& v_ ) {
        int idx = 0x0; 
        if( _sz == _cap ) [[likely]] {
            idx = _head++; _head %= _cap;
        } else {
            idx = ( _head + _sz++ ) % _cap;
        }
        return _data[ idx ] = v_;
    }

};


template< typename _prec_t_ = real_t >
vec< _prec_t_ > linspace_n( int n_, _prec_t_ low_, _prec_t_ upp_ ) {
    vec< _prec_t_ > span; span.assign( n_, _prec_t_{0} );
    mdn_0::linspace_n( span.data(), n_, low_, upp_ );
    return span;
}

template< typename _prec_t_ = real_t >
vec< _prec_t_ > linspace_s( _prec_t_ s_, _prec_t_ low_, _prec_t_ upp_ ) {
    int steps = ( int )(std::abs( upp_ - low_ ) / s_ );
    vec< _prec_t_ > span; span.assign( steps, _prec_t_{0} );
    mdn_0::linspace_n( span.data(), steps, low_, upp_ );
    return span;
}

template< typename _prec_t_ = real_t >
vec< std::complex< _prec_t_ > > roots( const vec< _prec_t_ >& co_ ) {
    const int n = (int)co_.size() - 1;
    _prec_t_ rr[ n ], ri[ n ];
    
    A113_ASSERT_OR( A113_OK == mdn_0::roots< _prec_t_ >( co_.data(), n, rr, ri ) ) return {};

    vec< std::complex< _prec_t_ > > res; res.reserve( n );
    for( int i = 0x0; i < n; ++i ) res.emplace_back( rr[ i ], ri[ i ] );
    return res;
}


template< typename _prec_t_ = real_t > struct siso_t {
public:
    virtual _prec_t_ step( _prec_t_ u_, _prec_t_ t_ ) = 0;
    virtual void reset( void ) = 0;
};

template< typename _prec_t_ = real_t > struct tfc_t : public siso_t< _prec_t_ > {
public:
    tfc_t( void ) = default;

    tfc_t( vec< _prec_t_ > num_, vec< _prec_t_ > den_, mdn_0::DiscDiffMth_ diffm_ ) {
        this->bind( std::move( num_ ), std::move( den_ ), diffm_ );
    }

public:
    vec< _prec_t_ >      den     = {};
    vec< _prec_t_ >      num     = {};   
    vec< _prec_t_ >      dy_h    = {};
    vec< _prec_t_ >      du_h    = {};
    mdn_0::DiscDiffMth_   diffm   = mdn_0::DiscDiffMth_FwdEuler;
    
public:
    status_t bind( vec< _prec_t_ > num_, vec< _prec_t_ > den_, mdn_0::DiscDiffMth_ diffm_ ) {
        A113_ASSERT_OR( not den_.empty() && not num_.empty() ) {
            BridgE->error( "tfc_t: den and num shall not be empty." );
            return A113_ERR_BADARG;
        }
        den   = std::move( den_ ); 
        num   = std::move( num_ );
        diffm = diffm_;

        switch( diffm ) {
            case mdn_0::DiscDiffMth_FwdEuler: {
                dy_h.assign( this->n() * 2, _prec_t_{0} );
                du_h.assign( this->m() * 2, _prec_t_{0} );
            break; }

            default: {
                BridgE->error( "tfc_t: unknown discrete differentiation method." );
                return A113_ERR_BADARG;
            }
        }

        return A113_OK;
    }

public:
    A113_inline int n( void ) const { return (int)den.size(); }
    A113_inline int m( void ) const { return (int)num.size(); }

public:
    virtual _prec_t_ step( _prec_t_ u_, _prec_t_ t_ ) override {
        switch( diffm ) {
            case mdn_0::DiscDiffMth_FwdEuler: return mdn_0::tfc_step_fwdeul( 
                den.data(), this->n(), num.data(), this->m(), dy_h.data(), du_h.data(), u_, t_
            );
        }

        BridgE->error( "tfc_t: unknown discrete differentiation method." );
        return 0x0;
    }

    virtual void reset( void ) override {
        std::fill_n( dy_h.data(), dy_h.size(), _prec_t_{0} );
        std::fill_n( du_h.data(), du_h.size(), _prec_t_{0} );
    }

};

}

/* === hyper structures === */
namespace a113::mdn_2 { using namespace _mdn;

using mdn_1::vec;

template< typename _prec_t_ = real_t > struct grid_t {
public:
    using apply_op_t = std::function< _prec_t_( _prec_t_* ) >;

public:
    grid_t( void ) = default;

public:
    vec< vec< _prec_t_ > >   _spans   = {};
    vec< _prec_t_ >          _field   = {};
    _prec_t_                 _min     = _prec_t_{0};
    _prec_t_                 _max     = _prec_t_{0};

public:
    A113_inline _prec_t_* raw( void ) { return _field.data(); }

    A113_inline int n_of( int d_ ) const { return (int)_spans[d_].size(); }
    A113_inline int count( void ) const { return (int)_field.size(); }
    A113_inline int dims( void ) const { return (int)_spans.size(); }

public:
    A113_inline void _align_field( void ) {
        int z_field_sz = 1;
        for( auto& span : _spans ) z_field_sz *= span.size();
        _field.assign( z_field_sz, _prec_t_{0} );
    }

public:
    grid_t& span_n( 
        const vec< std::tuple< int, _prec_t_, _prec_t_ > >& spans_
    ) {
        _spans.assign( spans_.size(), {} );
        for( int d = 0x0; d < spans_.size(); ++d ) {
            _spans[d] = mdn_1::linspace_n( 
                std::get< 0 >( spans_[d] ), std::get< 1 >( spans_[d] ), std::get< 2 >( spans_[d] ) 
            );
        }
        this->_align_field();
        return *this;
    }

    grid_t& span_s( 
        const vec< std::tuple< _prec_t_, _prec_t_, _prec_t_ > >& spans_
    ) {
        _spans.assign( spans_.size(), {} );
        for( int d = 0x0; d < spans_.size(); ++d ) {
            _spans[d] = mdn_1::linspace_s( 
                std::get< 0 >( spans_[d] ), std::get< 1 >( spans_[d] ), std::get< 2 >( spans_[d] ) 
            );
        }
        this->_align_field();
        return *this;
    }

public:
    _prec_t_& field_at( int* x_ ) {
        int offset = 0x0;

        for( int d = this->dims() - 1; d >= 0x1; --d ) {
            offset += x_[d] * this->n_of( d-1 ) + x_[d-1];
        }
        return _field[ offset ];
    }

    A113_inline _prec_t_& operator () ( int* x_ ) {
        return this->field_at( x_ );
    }

    A113_inline _prec_t_ min( void ) const { return _min; }
    A113_inline _prec_t_ max( void ) const { return _max; }

public:
    grid_t& apply( apply_op_t op_ ) {
        const int count       = this->count();
        const int dims        = this->dims();
        _prec_t_   x[ dims ]   = { _prec_t_{0} };
        int       ds[ count ] = { 0x0 };
        
        _min = std::numeric_limits< _prec_t_ >::max();
        _max = std::numeric_limits< _prec_t_ >::min();

        for( int i = 0x0; i < count; ++i ) {
            for( int d = 0x0; d < dims; ++d ) x[d] = _spans[d][ds[d]];
            _field[i] = op_( x );
            if( _field[i] > _max ) _max = _field[i];
            else if( _field[i] < _min ) _min = _field[i];

            for( int d = 0x0; d < dims - 1; ++d ) 
                if( ++ds[d] >= _spans[d].size() ) { ++ds[d+1]; ds[d] = 0x0; }
                else break;
        }

        return *this;
    } 

public:
    _prec_t_ MSE_with( const grid_t& other_ ) const {
        const int N = this->count();
        return _prec_t_{1.0}/N * mdn_0::roam_acc_2( _field.data(), other_._field.data(), N, _prec_t_{0}, [] ( _prec_t_ rhs, _prec_t_ lhs ) {
            return std::pow( rhs - lhs, 2 );
        } );
    }

public:
    std::pair< vec< float >, vec< unsigned int > > gen_VBO_and_EBO( int d0_ = 0x0, int d1_ = 0x1 ) {
        const vec< _prec_t_ >& x_span = _spans[ d0_ ];
        const vec< _prec_t_ >& y_span = _spans[ d1_ ];
        const int             xn     = x_span.size();
        const int             yn     = y_span.size();

        const int point_count = this->count();
        const int quad_count  = ( xn - 1 ) * ( yn - 1 );

        vec< float >        vbo{}; vbo.reserve( 3 * point_count );
        vec< unsigned int > ebo{}; ebo.reserve( 6 * quad_count );

        int z = 0x0;
        for( int y = 0x0; y < yn-1; ++y ) {
            for( int x = 0x0; x < xn-1; ++x ) {
                vbo.push_back( x_span[x] ); vbo.push_back( y_span[y] ); vbo.push_back( _field[z++] );

                unsigned int 
                base_ebo_idx = y * xn + x + 1; ebo.push_back( base_ebo_idx );
                base_ebo_idx -= 1;             ebo.push_back( base_ebo_idx );
                base_ebo_idx += xn;            ebo.push_back( base_ebo_idx );
                                               ebo.push_back( base_ebo_idx );
                base_ebo_idx += 1;             ebo.push_back( base_ebo_idx );
                base_ebo_idx -= xn;            ebo.push_back( base_ebo_idx );
            }
            vbo.push_back( x_span.back() ); vbo.push_back( y_span[y] ); vbo.push_back( _field[z++] );
        }
        for( int x = 0x0; x < xn; ++x ) {
            vbo.push_back( x_span[x] ); vbo.push_back( y_span.back() ); vbo.push_back( _field[z++] );
        }
      
        return { vbo, ebo };
    }

};

}