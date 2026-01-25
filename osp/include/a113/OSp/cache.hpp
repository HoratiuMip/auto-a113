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

template< typename _KEY_T_, typename _VALUE_T_ >
class Bucket {
public:
    Bucket( const bucket_init_args_t& args_ ) {}
    Bucket( void ) : Bucket{ bucket_init_args_t{} } {}

_A113_PROTECTED:
    std::map< _KEY_T_, HVec< _VALUE_T_ > >   _map   = {};
    mutable std::shared_mutex                _mtx   = {};
 
public:
    [[nodiscard]] HVec< _VALUE_T_ > query( const _KEY_T_& key_ ) const {
        std::shared_lock lck{ _mtx };
        auto itr = _map.find( key_ );
        return _map.end() == itr ? nullptr : itr->second;
    }

    void commit( const _KEY_T_& key_, _VALUE_T_&& value_ ) {
        std::unique_lock lck{ _mtx };
        _map.insert_or_assign( key_, std::move( value_ ) );
    }

};

}