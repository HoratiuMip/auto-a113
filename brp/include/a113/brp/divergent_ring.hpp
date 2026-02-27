#pragma once
/**
 * @file: brp/divergent_ring.hpp
 * @brief:
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/descriptor.hpp>

#include <atomic>

namespace a113 { namespace clst {

template< typename _T > class Divergent_ring {
public:
    struct slot_t {
         mutable std::atomic_int16_t   _ref    = { 0x0 };
         const int64_t                 _born   = 0x0;
    };

};

} };
