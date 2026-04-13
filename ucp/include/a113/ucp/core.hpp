#pragma once
/**
 * @file: ucp/core.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/gep/core.hpp>

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

#ifdef A113_TARGET_OS_FREERTOS
namespace a113::freertos_literals {

unsigned long long operator "" _pdms2t( unsigned long long ms_ ) { return pdMS_TO_TICKS(ms_); }

};
#endif
