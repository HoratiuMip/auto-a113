#pragma once
#include <a113/osp/core.hpp>

namespace a113::rxt_0 {

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

}

namespace a113::rxt_1 {

template< typename _FTYPE_v_ >
std::vector< _FTYPE_v_ > linspace_n( int n_, _FTYPE_v_ low_, _FTYPE_v_ upp_ ) {
    std::vector< _FTYPE_v_ > span; span.assign( n_, _FTYPE_v_{0x0} );
    rxt_0::linspace_n( span.data(), n_, low_, upp_ );
    return span;
}

template< typename _FTYPE_v_ >
std::vector< _FTYPE_v_ > linspace_s( _FTYPE_v_ s_, _FTYPE_v_ low_, _FTYPE_v_ upp_ ) {
    int steps = ( int )(( upp_ - low_ ) / s_ );
    std::vector< _FTYPE_v_ > span; span.assign( steps, _FTYPE_v_{0x0} );
    rxt_0::linspace_n( span.data(), steps, low_, upp_ );
    return span;
}

template< typename _FTYPE_ = double > struct srf_grid_t {
public:
    using apply_op_t = std::function< _FTYPE_( _FTYPE_, _FTYPE_ ) >;

public:
    srf_grid_t( void ) = default;

    srf_grid_t( 
        int xn_, _FTYPE_ xlow_, _FTYPE_ xupp_,
        int yn_, _FTYPE_ ylow_, _FTYPE_ yupp_
    ) {
        this->span_n( xn_, xlow_, xupp_, yn_, ylow_, yupp_ );
    }
    srf_grid_t( 
        _FTYPE_ xs_, _FTYPE_ xlow_, _FTYPE_ xupp_,
        _FTYPE_ ys_, _FTYPE_ ylow_, _FTYPE_ yupp_
    ) {
        this->span_s( xs_, xlow_, xupp_, ys_, ylow_, yupp_ );
    }

public:
    std::vector< _FTYPE_ >   _x_span   = {};
    std::vector< _FTYPE_ >   _y_span   = {};
    std::vector< _FTYPE_ >   _z_mat    = {};
    _FTYPE_                  _z_min    = _FTYPE_{0x0};
    _FTYPE_                  _z_max    = _FTYPE_{0x0};

public:
    A113_inline _FTYPE_* raw( void ) { return _z_mat.data(); }

    A113_inline int xn( void ) const { return (int)_x_span.size(); }
    A113_inline int yn( void ) const { return (int)_y_span.size(); }
    A113_inline int count( void ) const { return (int)_z_mat.size(); }

public:
    A113_inline void _align_z_mat( void ) {
        _z_mat.assign( this->xn() * this->yn(), _FTYPE_{0x0} );
    }

    void _apply_for_y( apply_op_t op_, int y1_, int y2_, _FTYPE_* z_min_, _FTYPE_* z_max_ ) {
        *z_min_ = *z_max_ = _z_mat[0x0];

        int z = 0x0;
        for( int y = y1_; y < y2_; ++y ) {
            for( int x = 0; x < this->xn(); ++x ) {
                _z_mat[z] = op_( _x_span[x], _y_span[y] );
                if( _z_mat[z] < *z_min_ ) *z_min_ = _z_mat[z];
                else if( _z_mat[z] > *z_max_ ) *z_max_ = _z_mat[z];
                ++z;
            }
        }
    }

public:
    A113_inline srf_grid_t& span_n( 
        int xn_, _FTYPE_ xlow_, _FTYPE_ xupp_, 
        int yn_, _FTYPE_ ylow_, _FTYPE_ yupp_ 
    ) {
        _x_span = linspace_n( xn_, xlow_, xupp_ );
        _y_span = linspace_n( yn_, ylow_, yupp_ );
        this->_align_z_mat();
        return *this;
    }

    A113_inline srf_grid_t& span_s( 
        _FTYPE_ xs_, _FTYPE_ xlow_, _FTYPE_ xupp_, 
        _FTYPE_ ys_, _FTYPE_ ylow_, _FTYPE_ yupp_
    ) {
        _x_span = linspace_s( xs_, xlow_, xupp_ );
        _y_span = linspace_s( ys_, ylow_, yupp_ );
        this->_align_z_mat();
        return *this;
    }

public:
    A113_inline _FTYPE_& z_at( int x_, int y_ ) {
        return _z_mat[ y_ * this->xn() + x_ ];
    }

    A113_inline _FTYPE_& operator () ( int x_, int y_ ) {
        return this->z_at( x_, y_ );
    }

    A113_inline _FTYPE_ min( void ) const { return _z_min; }
    A113_inline _FTYPE_ max( void ) const { return _z_max; }

public:
    srf_grid_t& apply( apply_op_t op_ ) {
        this->_apply_for_y( op_, 0, this->yn(), &_z_min, &_z_max );
        return *this;
    } 

    srf_grid_t& apply_spwn( int th_count_, apply_op_t op_ ) {
        const int y_count = this->yn();
        const int y_step  = (int)( y_count / th_count_ );
        int       y_crt   = 0;

        std::thread ths[ th_count_ ];
        for( auto& th : ths ) {
            th = std::thread( 
                &srf_grid_t< _FTYPE_ >::_apply_for_y, this, 
                op_, y_crt, std::min( y_count, y_crt + y_step ),
                nullptr, nullptr
            );
            y_crt += y_step;
        }
        for( auto& th : ths ) if( th.joinable() ) th.join();
        return *this;
    }

public:
    _FTYPE_ MSE_with( const srf_grid_t& other_ ) const {
        const int N = this->xn() * this->yn();
        return _FTYPE_{1.0}/N * rxt_0::roam_acc_2( _z_mat.data(), other_._z_mat.data(), N, _FTYPE_{0x0}, [] ( _FTYPE_ rhs, _FTYPE_ lhs ) {
            return std::pow( rhs - lhs, 2 );
        } );
    }

public:
    std::pair< std::vector< _FTYPE_ >, std::vector< unsigned int > > gen_VBO_and_EBO( void ) {
        const int point_count = this->count();
        const int quad_count  = ( this->xn() - 1 ) * ( this->yn() - 1 );

        std::vector< _FTYPE_ >      vbo{}; vbo.reserve( 3 * point_count );
        std::vector< unsigned int > ebo{}; ebo.reserve( 6 * quad_count );

        int z = 0x0;
        for( int y = 0x0; y < this->yn()-1; ++y ) {
            for( int x = 0x0; x < this->xn()-1; ++x ) {
                vbo.push_back( _x_span[x] ); vbo.push_back( _y_span[y] ); vbo.push_back( _z_mat[z++] );
                
                unsigned int 
                base_ebo_idx = y * this->xn() + x + 1; ebo.push_back( base_ebo_idx );
                base_ebo_idx -= 1;                     ebo.push_back( base_ebo_idx );
                base_ebo_idx += this->xn();            ebo.push_back( base_ebo_idx );
                                                       ebo.push_back( base_ebo_idx );
                base_ebo_idx += 1;                     ebo.push_back( base_ebo_idx );
                base_ebo_idx -= this->xn();            ebo.push_back( base_ebo_idx );
            }
            vbo.push_back( _x_span.back() ); vbo.push_back( _y_span[y] ); vbo.push_back( _z_mat[z++] );
        }
        for( int x = 0x0; x < this->xn(); ++x ) {
            vbo.push_back( _x_span[x] ); vbo.push_back( _y_span.back() ); vbo.push_back( _z_mat[z++] );
        }

        return { vbo, ebo };
    }

};

}