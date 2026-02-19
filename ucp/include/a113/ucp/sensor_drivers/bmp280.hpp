#pragma once
/**
 * @file: ucp/sensor_drivers/bmp280.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/ucp/IO_i2c_ints.hpp>

namespace a113::sdrv { using namespace a113::io;

class BMP280 {
public:
inline static constexpr   i2c_addr_t   I2C_ADDRESS_SDO_GND   = 0x76;
inline static constexpr   i2c_addr_t   I2C_ADDRESS_SDO_VCC   = 0x77;

public:
    enum CtrlMeas_ {
        CtrlMeas_TemperatureSampling_0x  = 0b00000000,
        CtrlMeas_TemperatureSampling_1x  = 0b00100000,
        CtrlMeas_TemperatureSampling_2x  = 0b01000000,
        CtrlMeas_TemperatureSampling_4x  = 0b01100000,
        CtrlMeas_TemperatureSampling_8x  = 0b10000000,
        CtrlMeas_TemperatureSampling_16x = 0b10100000,
        CtrlMeas_PressureSampling_0x     = 0b00000000,
        CtrlMeas_PressureSampling_1x     = 0b00000100,
        CtrlMeas_PressureSampling_2x     = 0b00001000,
        CtrlMeas_PressureSampling_4x     = 0b00001100,
        CtrlMeas_PressureSampling_8x     = 0b00010000,
        CtrlMeas_PressureSampling_16x    = 0b00010100,
        CtrlMeas_Power_Low               = 0b00000000,
        CtrlMeas_Power_OneShot           = 0b00000001,
        CtrlMeas_Power_Continous         = 0b00000011
    };
    enum Config_ {
        Config_Standby_5MS               = 0b00000000,
        Config_Standby_62MS5             = 0b00100000,
        Config_Standby_125MS             = 0b01000000,
        Config_Standby_250MS             = 0b01100000,
        Config_Standby_500MS             = 0b10000000,
        Config_Standby_1S                = 0b10100000,
        Config_Standby_2S                = 0b11000000,
        Config_Standby_4S                = 0b11100000,
        Config_Filter_Off                = 0b00000000,
        Config_Filter_2                  = 0b00000100,
        Config_Filter_4                  = 0b00001000,
        Config_Filter_8                  = 0b00001100,
        Config_Filter_16                 = 0b00010000
    };

public:
    typedef   int16_t    temperature_t;
    typedef   uint32_t   pressure_t;

    inline static constexpr temperature_t   INVALID_TEMPERATURE   = 0xFFFF;
    inline static constexpr pressure_t      INVALID_PRESSURE      = 0xFFFFFFFF;

public:
    BMP280( void ) = default;

_A113_PROTECTED:
    io::I2C_m2s_Int*   _i2c         = nullptr;

    struct _calibs_t {
        uint16_t   dig_T1   = 0x0;
        int16_t    dig_T2   = 0x0;
        int16_t    dig_T3   = 0x0;
        uint16_t   dig_P1   = 0x0;
        int16_t    dig_P2   = 0x0;
        int16_t    dig_P3   = 0x0;
        int16_t    dig_P4   = 0x0;
        int16_t    dig_P5   = 0x0;
        int16_t    dig_P6   = 0x0;
        int16_t    dig_P7   = 0x0;
        int16_t    dig_P8   = 0x0;
        int16_t    dig_P9   = 0x0;
    }                  _calibs      = {};
    int32_t            _fine_temp   = 0x0;

_A113_PROTECTED:
    temperature_t _calib_raw_temperature( int32_t raw_ ) {
        int32_t var1, var2;

        var1 = ((((raw_>>3) - ((int32_t)_calibs.dig_T1<<1))) * ((int32_t)_calibs.dig_T2)) >> 11;
        var2 = (((((raw_>>4) - ((int32_t)_calibs.dig_T1)) * ((raw_>>4) - ((int32_t)_calibs.dig_T1))) >> 12) * ((int32_t)_calibs.dig_T3)) >> 14;

        _fine_temp = var1 + var2;
        return (temperature_t)((_fine_temp * 5 + 128) >> 8);
    }

    pressure_t _calib_raw_pressure( int64_t raw_ ) {
        int64_t var1, var2;

        var1 = ((int64_t)_fine_temp) - 128000;
        var2 = var1 * var1 * (int64_t)_calibs.dig_P6;
        var2 = var2 + ((var1*(int64_t)_calibs.dig_P5)<<17);
        var2 = var2 + (((int64_t)_calibs.dig_P4)<<35);
        var1 = ((var1 * var1 * (int64_t)_calibs.dig_P3)>>8) + ((var1 * (int64_t)_calibs.dig_P2)<<12);
        var1 = (((((int64_t)1)<<47)+var1))*((int64_t)_calibs.dig_P1)>>33;

        A113_ASSERT_OR( 0 != var1 ) return INVALID_PRESSURE;

        raw_ = 1048576-raw_;
        raw_ = (((raw_<<31)-var2)*3125)/var1;
        var1 = (((int64_t)_calibs.dig_P9) * (raw_>>13) * (raw_>>13)) >> 25;
        var2 = (((int64_t)_calibs.dig_P8) * raw_) >> 19;
        return ((raw_ + var1 + var2)>>8) + (((int64_t)_calibs.dig_P7)<<4);
    }

public:
    status_t bind_i2c( I2C_m2s_Int* i2c_ ) { _i2c = i2c_; return A113_OK; }

public:
    status_t load_calibs( void ) {
        uint8_t buffer[ 24 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->write_read( 
            { .src_ptr = (char[]){ 0x88 }, .src_n = 1 }, 
            { .dst_ptr = (char*)buffer, .dst_n = sizeof( buffer ) } 
        ) );
    #define _MAKE_WORD( offset ) ( ( buffer[ offset+1 ] << 8 ) | buffer[ offset ] )
        _calibs.dig_T1 = _MAKE_WORD(0);
        _calibs.dig_T2 = _MAKE_WORD(2);
        _calibs.dig_T3 = _MAKE_WORD(4);
        _calibs.dig_P1 = _MAKE_WORD(6);
        _calibs.dig_P2 = _MAKE_WORD(8);
        _calibs.dig_P3 = _MAKE_WORD(10);
        _calibs.dig_P4 = _MAKE_WORD(12);
        _calibs.dig_P5 = _MAKE_WORD(14);
        _calibs.dig_P6 = _MAKE_WORD(16);
        _calibs.dig_P7 = _MAKE_WORD(18);
        _calibs.dig_P8 = _MAKE_WORD(20);
        _calibs.dig_P9 = _MAKE_WORD(22);
    #undef _MAKE_WORD
        return A113_OK;
    }

    status_t load_data( temperature_t* temp_out_, pressure_t* press_out_ ) {
        uint8_t buffer[ 6 ];

        A113_ASSERT_STATUS_OR_RET( _i2c->write_read( 
            { .src_ptr = (char[]){ 0xF7 }, .src_n = 1 }, 
            { .dst_ptr = (char*)buffer, .dst_n = sizeof( buffer ) } 
        ) );
  
        int32_t raw_temp  = ( int32_t )( ( buffer[ 3 ] << 12 ) | ( buffer[ 4 ] << 4 ) | ( buffer[ 5 ] >> 4 ) );
        int64_t raw_press = ( int32_t )( ( buffer[ 0 ] << 12 ) | ( buffer[ 1 ] << 4 ) | ( buffer[ 2 ] >> 4 ) );

        auto temp = this->_calib_raw_temperature( raw_temp );
        if( temp_out_ )  *temp_out_  = temp;
        if( press_out_ ) *press_out_ = this->_calib_raw_pressure( raw_press );

        return A113_OK;
    }

    status_t load_data( float* temp_out_, float* press_out_ ) {
        temperature_t temp  = INVALID_TEMPERATURE;
        pressure_t    press = INVALID_PRESSURE;
        
        A113_ASSERT_STATUS_OR_RET( this->load_data( &temp, &press ) );

        if( temp_out_ )  *temp_out_  = (float)temp / 100;
        if( press_out_ ) *press_out_ = (float)press / 256;

        return A113_OK;
    }

public:
    A113_inline status_t store_ctrl_meas( uint8_t val_ ) { return _i2c->write_reg( 0xF4, val_ ); }
    A113_inline status_t store_config( uint8_t val_ ) { return _i2c->write_reg( 0xF5, val_ ); }
};

};