#pragma once
/**
 * @file: osp/text_utils.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/gep/core.hpp>

namespace a113::text {

/**
 * @brief Levenshtein distance between two strings each of any length.
 */
int lev_dist( std::string_view s_, std::string_view t_ );


constexpr uint32_t hash( const std::string& str_ ) {
    uint32_t h = 2166136261U;
    for( char c : str_ ) {
        h ^= (uint32_t)c;
        h *= 16777619U;
    }
    return h;
}

};
