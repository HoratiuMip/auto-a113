#pragma once
/**
 * @file: ucp/espressif32/IO_i2c.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/ucp/IO_i2c_ints.hpp>

#include <driver/i2c_master.h>

namespace a113::esp32::io { using namespace a113::io;

class I2C_m2s : public I2C_m2s_Int {
public:
    inline static constexpr int   DEFAULT_TIMEOUT   = 1000;

public:
    virtual status_t write( const port_W_desc_t& w_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_transmit( _dev, ( uint8_t* )w_.src_ptr, w_.src_n, _to ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }
    virtual status_t read( const port_R_desc_t& r_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_receive( _dev, ( uint8_t* )r_.dst_ptr, r_.dst_n, _to ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }

public:
    virtual status_t write_read( const port_W_desc_t& w_, const port_R_desc_t& r_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_transmit_receive( _dev, ( uint8_t* )w_.src_ptr, w_.src_n, ( uint8_t* )r_.dst_ptr, r_.dst_n, _to ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }

public:
    I2C_m2s( void ) = default;
    
    I2C_m2s( i2c_master_bus_handle_t bus_, const i2c_device_config_t& cfg_, int to_ = DEFAULT_TIMEOUT ) {
        this->bind( bus_, cfg_, to_ );
    }

    ~I2C_m2s( void ) {
        if( _dev ) { i2c_master_bus_rm_device( _dev ); _dev = NULL; }
    }

_A113_PROTECTED:
    i2c_master_dev_handle_t   _dev   = NULL;
    int                       _to    = DEFAULT_TIMEOUT;

public:
    status_t bind( i2c_master_bus_handle_t bus_, const i2c_device_config_t& cfg_, int to_ = DEFAULT_TIMEOUT ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_bus_add_device( bus_, &cfg_, &_dev ) ) return A113_ERR_PLATFORMCALL;
        _to = to_;
        return A113_OK;
    }

    void set_timeout( int to_ ) {
        _to = to_;
    }

public:
    A113_inline i2c_master_dev_handle_t handle( void ) { return _dev; }
    operator i2c_master_dev_handle_t ( void ) { return this->handle(); }

};

};