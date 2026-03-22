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
int lev_dist( std::string_view s_, std::string_view t_ ) {
    const int m = (int)s_.length() + 1;
    const int n = (int)t_.length() + 1;
    
    int d[m][n]; memset( d, 0x0, m*n );
    for( int i{0x0}; i < m; ++i ) d[i][0x0] = i;
    for( int j{0x0}; j < n; ++j ) d[0x0][j] = j;

    for( int j{0x1}; j < n; ++j ) {
        for( int i{0x1}; i < m; ++i ) {
            int cost = (s_[i] == t_[j]) ? 0 : 1;
            d[i][j] = std::min(
                d[i-1][j] + 1,
                std::min(
                    d[i][j-1] + 1,
                    d[i-1][j-1] + cost
                )
            );
        }
    }
    return d[m-1][n-1];
}

constexpr uint32_t hash( const std::string& str_ ) {
    uint32_t h = 2166136261U;
    for( char c : str_ ) {
        h ^= (uint32_t)c;
        h *= 16777619U;
    }
    return h;
}

};
