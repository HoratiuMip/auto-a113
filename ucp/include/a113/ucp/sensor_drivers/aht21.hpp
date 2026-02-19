#pragma once
/**
 * @file: ucp/sensor_drivers/aht21.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/ucp/IO_i2c_ints.hpp>

namespace a113::sdrv { using namespace a113::io;

class AHT21 {
public:
    typedef   float   temperature_t;
    typedef   float   humidity_t;

public:
    inline static constexpr   i2c_addr_t      I2C_ADDRESS           = 0x38;
    inline static constexpr   int             MEASUREMENT_TIME_MS   = 300;
    inline static constexpr   temperature_t   INVALID_TEMPERATURE   = 0x1p128f;
    inline static constexpr   humidity_t      INVALID_PRESSURE      = 0x1p128f;

public:
    AHT21( void ) = default;

_A113_PROTECTED:
    io::I2C_m2s_Int*   _i2c   = nullptr;

public:
    status_t bind_i2c( I2C_m2s_Int* i2c_ ) { _i2c = i2c_; return A113_OK; }

public:
    A113_inline status_t calib( void ) {
        return _i2c->write( 
            { .src_ptr = (char[]){ 0xBE, 0b00001000, 0x00 }, .src_n = 3 }
        );
    }

    A113_inline status_t one_shot( void ) { 
        return _i2c->write( 
            { .src_ptr = (char[]){ 0xAC, 0x33, 0x00 }, .src_n = 3 }
        );
    }

    status_t load_data( temperature_t* temp_out_, humidity_t* hum_out_ ) {
        uint8_t buffer[ 6 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->read( 
            { .dst_ptr = (char*)buffer, .dst_n = sizeof( buffer ) }
        ) );

        A113_ASSERT_OR( RESET == (buffer[ 0 ] & 0x80) ) return A113_ERR_BUSY;
    
        uint32_t raw_temp = (uint32_t)(((buffer[ 3 ] & 0x0F) << 16) | (buffer[ 4 ] << 8) | buffer[ 5 ]);
        uint32_t raw_hum  = (uint32_t)((buffer[ 1 ] << 12) | (buffer[ 2 ] << 4) | (buffer[ 3 ] >> 4));

        if( temp_out_ ) *temp_out_ = ((float)raw_temp * 200) / (1 << 20) - 50;
        if( hum_out_ )  *hum_out_  = ((float)raw_hum * 100) / (1 << 20);

        return A113_OK;
    }
};

};