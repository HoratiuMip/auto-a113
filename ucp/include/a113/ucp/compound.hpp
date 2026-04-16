#pragma once
/**
 * @file: ucp/compound.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <a113/gep/compound.hpp>

#ifdef A113_TARGET_OS_FREERTOS
namespace a113 {

class CompoundCluster_FreeRTOS : public CompoundCluster {
public:
    struct config_t {
        int           iterate_interval_ms   = 5000;
        const char*   task_name             = "a113/cmpdclst";
        int           task_stack_depth      = 4096;
        UBaseType_t   task_priority         = configMAX_PRIORITIES - 1;
    };

_A113_PROTECTED:
    config_t       _config     = {};
    TaskHandle_t   _tsk_main   = NULL;

_A113_PROTECTED:
    bool _main_should_stop( void ) {
        return this->went_critical();
    }

_A113_PROTECTED:
    static void _main( void* arg_ ) {
        auto* self = ( CompoundCluster_FreeRTOS* )arg_;

    for(; not self->_main_should_stop() ;) {
        vTaskDelay( pdMS_TO_TICKS( self->_config.iterate_interval_ms ) );
        A113_ASSERT_OR( A113_OK == self->iterate_register() ) break;
    }
        vTaskDelete( NULL );
    }

public:
    status_t init( const config_t& config_ ) {
        _config = config_;

        A113_ASSERT_OR( pdPASS == xTaskCreate(
            &CompoundCluster_FreeRTOS::_main, _config.task_name, _config.task_stack_depth, this, _config.task_priority, &_tsk_main
        ) ) {
            return A113_ERR_SYSCALL;
        }
        return A113_OK;
    }

};

};
#endif