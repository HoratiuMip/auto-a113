#pragma once
/**
 * @file: ucp/esp32-4x/IO_i2c.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/ucp/IO_i2c_ifs.hpp>
#include <driver/i2c.h>

namespace a113::esp32::io { using namespace a113::io;

class I2C_m2s : public I2C_m2s_If {
public:
    inline static constexpr int   DEFAULT_TIMEOUT   = 1000;

public:
    virtual status_t write( const port_W_desc_t& w_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_write_to_device( _dev.bus, _dev.addr, (uint8_t*)w_.src_ptr, w_.src_n, pdMS_TO_TICKS(_to) ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }
    virtual status_t read( const port_R_desc_t& r_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_read_from_device( _dev.bus, _dev.addr, (uint8_t*)r_.dst_ptr, r_.dst_n, pdMS_TO_TICKS(_to) ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }

public:
    virtual status_t write_read( const port_W_desc_t& w_, const port_R_desc_t& r_ ) {
        A113_ASSERT_OR( ESP_OK == i2c_master_write_read_device( _dev.bus, _dev.addr, (uint8_t*)w_.src_ptr, w_.src_n, (uint8_t*)r_.dst_ptr, r_.dst_n, pdMS_TO_TICKS(_to) ) ) return A113_ERR_PLATFORMCALL;
        return A113_OK;
    }

public:
    I2C_m2s( void ) = default;
    
    I2C_m2s( i2c_port_t bus_, i2c_addr_t addr_, int to_ = DEFAULT_TIMEOUT ) {
        this->bind( bus_, addr_, to_ );
    }

_A113_PROTECTED:
    struct _dev_t {
        i2c_port_t   bus    = {};
        i2c_addr_t   addr   = 0x0;
    }     _dev  = {};
    int   _to   = DEFAULT_TIMEOUT;

public:
    status_t bind( i2c_port_t bus_, i2c_addr_t addr_, int to_ = DEFAULT_TIMEOUT ) {
        _dev = _dev_t{
            .bus  = bus_,
            .addr = addr_ 
        };
        _to = to_;
        return A113_OK;
    }

    void set_timeout( int to_ ) {
        _to = to_;
    }

public:
    A113_inline i2c_port_t bus( void ) { return _dev.bus; }
    operator i2c_port_t ( void ) { return this->bus(); }

};

};