namespace a113::rxt_0 {

template< typename _Tv_, typename _Tb_ >
_Tb_ linspace_n( _Tv_* v_, int n_, _Tb_ low_, _Tb_ upp_ ) {
    _Tb_ step = ( upp_ - low_ ) / n_;
    for( int n = 0; n < n_; ++n ) {
        v_[ n ] = (_Tv_)low_;
        low_ += step;
    } 
    return step;
}

template< typename _T1_, typename _T2_, typename _Tr_, typename _Op_ >
_Tr_ roam_acc_2( _T1_* v1_, _T2_* v2_, int n_, _Tr_ acc_, _Op_ op_ ) {
    for( int i = 0; i < n_; ++i ) acc_ += op_( v1_[i], v2_[i] );
    return acc_;
}

}