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

namespace a113 {

struct Service {
public:
    enum STATE_ {
        STATE_STOPPED, STATE_STARTED, STATE_STOPPING, STATE_STARTING, STATE_FAULT
    };

_A113_PROTECTED:
    std::atomic< STATE_ >   _service_state   = { STATE_STOPPED };

_A113_PROTECTED:
    virtual status_t _service_start( void* ctx_ ) = 0;

    virtual status_t _service_stop( void* ctx_ ) = 0;

public:
    status_t service_start( void* ctx_ ) {
        _service_state.store( STATE_STARTING, std::memory_order_release );
        status_t status = this->_service_start( ctx_ );
        A113_ASSERT_OR( A113_OK == status ) {
            _service_state.store( STATE_FAULT, std::memory_order_release );
            return status;
        }
        _service_state.store( STATE_STARTED, std::memory_order_release );
        return A113_OK;
    }

    status_t service_stop( void* ctx_ ) {
        _service_state.store( STATE_STOPPING, std::memory_order_release );
        status_t status = this->_service_stop( ctx_ );
        A113_ASSERT_OR( A113_OK == status ) {
            _service_state.store( STATE_FAULT, std::memory_order_release );
            return status;
        }
        _service_state.store( STATE_STOPPED, std::memory_order_release );
        return A113_OK;
    }

    virtual status_t service_restart( void* ctxa_, void* ctxb_ ) {
        A113_ASSERT_STATUS_OR_RET( this->service_stop( ctxa_ ) );
        return this->service_start( ctxb_ );
    }
};

};
