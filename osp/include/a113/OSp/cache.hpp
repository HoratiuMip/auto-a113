#pragma once
/**
 * @file: ops/cache.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>

namespace a113::cache {

struct bucket_init_args_t {

};

enum BucketHandle_ : int {
    BucketHandle_None    = 0x0,
    BucketHandle_Bypass  = ( 0x1 << 0 ),
    BucketHandle_Queried = ( 0x1 << 1 ),
    BucketHandle_Disable = BucketHandle_Bypass | BucketHandle_Queried
};

template< typename _KEY_T_, typename _VALUE_T_ >
class Bucket {
public:
    Bucket( const bucket_init_args_t& args_ ) {}
    Bucket( void ) : Bucket{ bucket_init_args_t{} } {}

_A113_PROTECTED:
    std::map< _KEY_T_, HVec< _VALUE_T_ > >   _map   = {};
    mutable std::shared_mutex                _mtx   = {};
 
public:
    [[nodiscard]] HVec< _VALUE_T_ > query( const _KEY_T_& key_, BucketHandle_& hdl_ ) const {
        if( hdl_ & BucketHandle_Queried ) return nullptr;

        std::shared_lock lck{ _mtx };
        auto itr = _map.find( key_ );

        hdl_ = (BucketHandle_)( hdl_ | BucketHandle_Queried );
        return _map.end() == itr ? nullptr : itr->second;
    }

    HVec< _VALUE_T_ > commit( const _KEY_T_& key_, _VALUE_T_&& value_, BucketHandle_& hdl_ ) {
        auto element = HVec< _VALUE_T_ >::make( std::move( value_ ) );
        if( hdl_ & BucketHandle_Bypass ) return element;

        std::unique_lock lck{ _mtx };
        _map.insert_or_assign( key_, element );

        return element;
    }

    void commit( const _KEY_T_& key_, const HVec< _VALUE_T_ >& value_ ) {
        std::unique_lock lck{ _mtx };
        _map.insert_or_assign( key_, value_ );
    }

};

}