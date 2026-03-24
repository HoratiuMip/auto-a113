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
#include <complex>
/* === excom includes === */
#include <lapacke.h>

namespace a113::mdn {

class _bridge_t {
public:
    _bridge_t( void ) {
        logger = spdlog::stdout_color_mt( A113_VERSION_STRING"/madonna" ); 
        logger->set_pattern( A113_SPDLOG_PATTERN );

        logger->info( "Bridge init OK." );
    }

_A113_PROTECTED:
    HVec< spdlog::logger >   logger   = nullptr;

public:
    A113_inline spdlog::logger* operator -> ( void ) { return logger.get(); }

}; inline _bridge_t BridgE;

};

namespace a113::mdn_0 { using namespace mdn;

enum DiscDiffMth_ {
    DiscDiffMth_FwdEuler,
    DiscDiffMth_Tustin
};

template< typename _FTYPE_ > struct tfc_t {
    int        n     = 0;
    int        m     = 0;
    _FTYPE_*   den   = nullptr;
    _FTYPE_*   num   = nullptr;
};

template< typename _FTYPE_v_, typename _FTYPE_b_ >
_FTYPE_b_ linspace_n( _FTYPE_v_* v_, int n_, _FTYPE_b_ low_, _FTYPE_b_ upp_ ) {
    _FTYPE_b_ step = ( upp_ - low_ ) / n_;
    for( int n = 0; n < n_; ++n ) {
        v_[ n ] = (_FTYPE_v_)low_;
        low_ += step;
    } 
    return step;
}
 
template< typename _FTYPE_1_, typename _FTYPE_2_, typename _FTYPE_r_, typename _Op_ >
_FTYPE_r_ roam_acc_2( _FTYPE_1_* v1_, _FTYPE_2_* v2_, int n_, _FTYPE_r_ acc_, _Op_ op_ ) {
    for( int i = 0; i < n_; ++i ) acc_ += op_( v1_[i], v2_[i] );
    return acc_;
}

template< typename _FTYPE_v_ >
status_t roots( const _FTYPE_v_* co_, int n_, _FTYPE_v_* rr_, _FTYPE_v_* ri_ ) {
    _FTYPE_v_ A[ n_ ][ n_ ]; memset( A, 0x0, n_*n_*sizeof( _FTYPE_v_ ) );

    for( int i = 0x0; i < n_; ++i ) {
        A[ 0x0 ][ i ] = -co_[ i+1 ] / co_[ 0x0 ];
        if( i != n_-1 ) A[ i+1 ][ i ] = _FTYPE_v_{1};
    }

    int info;   
    if constexpr( std::is_same_v< float, _FTYPE_v_ > ) {
        info = LAPACKE_sgeev( LAPACK_ROW_MAJOR, 'N', 'N', n_, (float*)A, n_, rr_, ri_, nullptr, 1, nullptr, 1 );
    } else if constexpr( std::is_same_v< double, _FTYPE_v_ > ) {
        info = LAPACKE_dgeev( LAPACK_ROW_MAJOR, 'N', 'N', n_, (double*)A, n_, rr_, ri_, nullptr, 1, nullptr, 1 );
    }
    A113_ASSERT_OR( info == 0x0 ) return A113_ERR_EXCOMCALL;
    return A113_OK;
}   

template< typename _FTYPE_v_ >
_FTYPE_v_ tfc_step_fwdeul( _FTYPE_v_* den_, int n_, _FTYPE_v_* num_, int m_, _FTYPE_v_* dy_h_, _FTYPE_v_* du_h_, _FTYPE_v_ u_, _FTYPE_v_ t_ ) {
    _FTYPE_v_ dny = num_[ m_-1 ] * u_;

    du_h_[ 0x1 ] = du_h_[ 0x0 ];
    du_h_[ 0x0 ] = u_;

    int ui = 0x0;
    for( int ord = 1; ord < m_; ++ord ) {
        const int i       = ord<<1;
        const _FTYPE_v_ d = ( du_h_[ ui ] - du_h_[ ui+1 ] );

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
        const _FTYPE_v_ d = dy_h_[ li ];

        dy_h_[ i+1 ] = dy_h_[ i ];
        dy_h_[ i ]   += d*t_;

        li = i;
    }

    return dy_h_[ 0x0 ];
}

}

namespace a113::mdn_1 { using namespace mdn;

template< typename _FTYPE_v_ > using vec = std::vector< _FTYPE_v_ >;

template< typename _FTYPE_ > struct tfc_t {
public:
    tfc_t( void ) = default;

    tfc_t( vec< _FTYPE_ > num_, vec< _FTYPE_ > den_, mdn_0::DiscDiffMth_ diffm_ ) {
        this->bind( std::move( num_ ), std::move( den_ ), diffm_ );
    }

public:
    vec< _FTYPE_ >        den     = {};
    vec< _FTYPE_ >        num     = {};   
    vec< _FTYPE_ >        dy_h    = {};
    vec< _FTYPE_ >        du_h    = {};
    mdn_0::DiscDiffMth_   diffm   = mdn_0::DiscDiffMth_FwdEuler;
    
public:
    status_t bind( vec< _FTYPE_ > num_, vec< _FTYPE_ > den_, mdn_0::DiscDiffMth_ diffm_ ) {
        A113_ASSERT_OR( not den_.empty() && not num_.empty() ) {
            BridgE->error( "tfc_t: den and num shall not be empty." );
            return A113_ERR_BADARG;
        }
        den   = std::move( den_ ); 
        num   = std::move( num_ );
        diffm = diffm_;

        switch( diffm ) {
            case mdn_0::DiscDiffMth_FwdEuler: {
                dy_h.assign( this->n() * 2, _FTYPE_{0} );
                du_h.assign( this->m() * 2, _FTYPE_{0} );
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
    _FTYPE_ step( _FTYPE_ u_, _FTYPE_ t_ ) {
        switch( diffm ) {
            case mdn_0::DiscDiffMth_FwdEuler: return mdn_0::tfc_step_fwdeul( 
                den.data(), this->n(), num.data(), this->m(), dy_h.data(), du_h.data(), u_, t_
            );
        }

        BridgE->error( "tfc_t: unknown discrete differentiation method." );
        return 0x0;
    }

};

template< typename _FTYPE_v_ >
vec< _FTYPE_v_ > linspace_n( int n_, _FTYPE_v_ low_, _FTYPE_v_ upp_ ) {
    vec< _FTYPE_v_ > span; span.assign( n_, _FTYPE_v_{0} );
    mdn_0::linspace_n( span.data(), n_, low_, upp_ );
    return span;
}

template< typename _FTYPE_v_ >
vec< _FTYPE_v_ > linspace_s( _FTYPE_v_ s_, _FTYPE_v_ low_, _FTYPE_v_ upp_ ) {
    int steps = ( int )(( upp_ - low_ ) / s_ );
    vec< _FTYPE_v_ > span; span.assign( steps, _FTYPE_v_{0} );
    mdn_0::linspace_n( span.data(), steps, low_, upp_ );
    return span;
}

template< typename _FTYPE_v_ >
vec< std::complex< _FTYPE_v_ > > roots( const vec< _FTYPE_v_ >& co_ ) {
    const int n = (int)co_.size() - 1;
    _FTYPE_v_ rr[ n ], ri[ n ];
    
    A113_ASSERT_OR( A113_OK == mdn_0::roots< _FTYPE_v_ >( co_.data(), n, rr, ri ) ) return {};

    vec< std::complex< _FTYPE_v_ > > res; res.reserve( n );
    for( int i = 0x0; i < n; ++i ) res.emplace_back( rr[ i ], ri[ i ] );
    return res;
}

}

namespace a113::mdn_2 { using namespace mdn;

using mdn_1::vec;

template< typename _FTYPE_ = double > struct srf_grid_t {
public:
    using apply_op_t = std::function< _FTYPE_( _FTYPE_* ) >;

public:
    srf_grid_t( void ) = default;

public:
    vec< vec< _FTYPE_ > >   _spans   = {};
    vec< _FTYPE_ >          _field   = {};
    _FTYPE_                 _min     = _FTYPE_{0};
    _FTYPE_                 _max     = _FTYPE_{0};

public:
    A113_inline _FTYPE_* raw( void ) { return _field.data(); }

    A113_inline int n_of( int d_ ) const { return (int)_spans[d_].size(); }
    A113_inline int count( void ) const { return (int)_field.size(); }
    A113_inline int dims( void ) const { return (int)_spans.size(); }

public:
    A113_inline void _align_field( void ) {
        int z_field_sz = 1;
        for( auto& span : _spans ) z_field_sz *= span.size();
        _field.assign( z_field_sz, _FTYPE_{0} );
    }

public:
    srf_grid_t& span_n( 
        const vec< std::tuple< int, _FTYPE_, _FTYPE_ > >& spans_
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

    srf_grid_t& span_s( 
        const vec< std::tuple< _FTYPE_, _FTYPE_, _FTYPE_ > >& spans_
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
    _FTYPE_& field_at( int* x_ ) {
        int offset = 0x0;

        for( int d = this->dims() - 1; d >= 0x1; --d ) {
            offset += x_[d] * this->n_of( d-1 ) + x_[d-1];
        }
        return _field[ offset ];
    }

    A113_inline _FTYPE_& operator () ( int* x_ ) {
        return this->field_at( x_ );
    }

    A113_inline _FTYPE_ min( void ) const { return _min; }
    A113_inline _FTYPE_ max( void ) const { return _max; }

public:
    srf_grid_t& apply( apply_op_t op_ ) {
        const int count       = this->count();
        const int dims        = this->dims();
        _FTYPE_   x[ dims ]   = { _FTYPE_{0} };
        int       ds[ count ] = { 0x0 };
        
        _min = _max = _field[0x0];

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
    _FTYPE_ MSE_with( const srf_grid_t& other_ ) const {
        const int N = this->count();
        return _FTYPE_{1.0}/N * mdn_0::roam_acc_2( _field.data(), other_._field.data(), N, _FTYPE_{0}, [] ( _FTYPE_ rhs, _FTYPE_ lhs ) {
            return std::pow( rhs - lhs, 2 );
        } );
    }

public:
    std::pair< vec< float >, vec< unsigned int > > gen_VBO_and_EBO( int d0_ = 0x0, int d1_ = 0x1 ) {
        const vec< _FTYPE_ >& x_span = _spans[ d0_ ];
        const vec< _FTYPE_ >& y_span = _spans[ d1_ ];
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