#pragma once
/**
 * @file: ucp/core.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/descriptor.hpp>

#ifndef HIGH
    #define HIGH 0x1
#elif 0x1 != HIGH
    #error "[A113-UCP] - HIGH should be defined as 1."
#endif
#ifndef LOW
    #define LOW 0x0
#elif 0x0 != LOW
    #error "[A113-UCP] - LOW should be defined as 0."
#endif

namespace rnk {



};