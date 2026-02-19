#pragma once
/**
 * @file: ucp/sensor_drivers/bme280.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/ucp/IO_i2c_ints.hpp>
#include <a113/ucp/sensor_drivers/bmp280.hpp>

namespace a113::sdrv { using namespace a113::io;

class BME280 : public BMP280 {
public:
    enum CtrlHum_ {
        CtrlHum_HumiditySampling_0x  = 0b00000000,
        CtrlHum_HumiditySampling_1x  = 0b00000001,
        CtrlHum_HumiditySampling_2x  = 0b00000010,
        CtrlHum_HumiditySampling_4x  = 0b00000011,
        CtrlHum_HumiditySampling_8x  = 0b00000100,
        CtrlHum_HumiditySampling_16x = 0b00000101
    };

public:
    typedef   uint32_t   humidity_t;

    inline static constexpr humidity_t   INVALID_HUMIDITY   = 0x1p128f;

public:
    BME280( void ) = default;

_A113_PROTECTED:
    struct _calibs_t {
        uint8_t    dig_H1   = 0x0;
        int16_t    dig_H2   = 0x0;
        int16_t    dig_H3   = 0x0;
        int16_t    dig_H4   = 0x0;
        int16_t    dig_H5   = 0x0;
        int16_t    dig_H6   = 0x0;
    }   _calibs   = {};

_A113_PROTECTED:
    humidity_t _calib_raw_humidity( int32_t raw_ ) {
        int32_t v_x1_u32r;
        v_x1_u32r = (_fine_temp - ((int32_t)76800));
        v_x1_u32r = (
        ((((raw_ << 14) - (((int32_t)_calibs.dig_H4) << 20) - (((int32_t)_calibs.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) 
        * 
        (((((((v_x1_u32r * ((int32_t)_calibs.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)_calibs.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)_calibs.dig_H2) + 8192) >> 14)
        );
        v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)_calibs.dig_H1)) >> 4));
        v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
        v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
        
        return v_x1_u32r >> 12;
    }

public:
    status_t load_calibs( void ) {
        A113_ASSERT_STATUS_OR_RET( this->BMP280::load_calibs() );

        uint8_t buffer[ 7 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->write_read( 
            { .src_ptr = (char[]){ 0xA1 }, .src_n = 1 }, 
            { .dst_ptr = (char*)buffer, .dst_n = 1 } 
        ) );
        _calibs.dig_H1 = buffer[ 0 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->write_read( 
            { .src_ptr = (char[]){ 0xE1 }, .src_n = 1 }, 
            { .dst_ptr = (char*)buffer, .dst_n = sizeof( buffer ) } 
        ) );
        _calibs.dig_H2 = (buffer[ 1 ] << 8) | buffer[ 0 ];
        _calibs.dig_H3 = buffer[ 2 ];
        _calibs.dig_H4 = (buffer[ 3 ] << 4) | (buffer[ 4 ] & 0x0F);
        _calibs.dig_H5 = (buffer[ 5 ] << 4) | (buffer[ 4 ] & 0xF0);
        _calibs.dig_H6 = buffer[ 6 ];

        return A113_OK;
    }

    status_t load_data( temperature_t* temp_out_, pressure_t* press_out_, humidity_t* hum_out_ ) {
        uint8_t buffer[ 8 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->write_read( 
            { .src_ptr = (char[]){ 0xF7 }, .src_n = 1 }, 
            { .dst_ptr = (char*)buffer, .dst_n = sizeof( buffer ) } 
        ) );
  
        int32_t raw_temp  = (int32_t)((buffer[ 3 ] << 12) | (buffer[ 4 ] << 4) | (buffer[ 5 ] >> 4));
        int64_t raw_press = (int32_t)((buffer[ 0 ] << 12) | (buffer[ 1 ] << 4) | (buffer[ 2 ] >> 4));
        int32_t raw_hum   = (int32_t)((buffer[ 6 ] << 8) | buffer[ 7 ]);

        auto temp = this->_calib_raw_temperature( raw_temp );
        if( temp_out_ )  *temp_out_  = temp;
        if( press_out_ ) *press_out_ = this->_calib_raw_pressure( raw_press );
        if( hum_out_ )   *hum_out_   = this->_calib_raw_humidity( raw_hum );
   
        return A113_OK;
    }

    status_t load_data( float* temp_out_, float* press_out_, float* hum_out_ ) {
        temperature_t temp  = INVALID_TEMPERATURE;
        pressure_t    press = INVALID_PRESSURE;
        humidity_t    hum   = INVALID_HUMIDITY;
        
        A113_ASSERT_STATUS_OR_RET( this->load_data( &temp, &press, &hum ) );

        if( temp_out_ )  *temp_out_  = (float)temp / 100;
        if( press_out_ ) *press_out_ = (float)press / 256;
        if( hum_out_ )   *hum_out_   = (float)hum / 1024;

        return A113_OK;
    }

public:
    A113_inline status_t store_ctrl_hum( uint8_t val_ ) { return _i2c->write_reg( 0xF2, val_ ); }

};

};