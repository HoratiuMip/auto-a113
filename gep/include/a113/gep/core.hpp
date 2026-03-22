#pragma once
/**
 * @file: gep/core.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/descriptor.hpp>

#include <any>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <stack>
#include <string>
#include <string_view>
#include <system_error>
#include <queue>

namespace a113 {

template< typename _T_ >
struct HVec : public std::shared_ptr< _T_ > {
    using std::shared_ptr< _T_ >::shared_ptr;
    using std::shared_ptr< _T_ >::operator=;

    HVec( const std::shared_ptr< _T_ >&  ptr_ ) : std::shared_ptr< _T_ >{ ptr_ } {}
    HVec( std::shared_ptr< _T_ >&& ptr_ ) : std::shared_ptr< _T_ >{ std::move( ptr_ ) } {}
    HVec( _T_* ptr_ ) { this->reset( ptr_ ); }
    HVec( _T_& ref_ ) : std::shared_ptr< _T_ >{ &ref_, [] ( _T_* ) static -> void {} } {}

    template< typename ...Args_ >
    A113_inline static HVec< _T_ > make( Args_&&... args_ ) { return std::make_shared< _T_ >( std::forward< Args_ >( args_ )... ); }
};

};