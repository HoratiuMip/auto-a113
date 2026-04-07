#pragma once
/**
 * @file: ucp/IO_i2c_ifs.hpp
 * @brief: 
 * @details:
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/IO_port.hpp>
#include <a113/ucp/core.hpp>

namespace a113::io {

class I2C_m2s_If : public Port {
public:
    virtual status_t write( const port_W_desc_t& w_ ) A113_UNIMPLEMENTED
    virtual status_t read( const port_R_desc_t& r_ ) A113_UNIMPLEMENTED

public:
    virtual status_t write_read( const port_W_desc_t& w_, const port_R_desc_t& r_ ) A113_UNIMPLEMENTED

public:
    A113_inline status_t write_reg( uint8_t reg_, uint8_t val_ ) {
    #if __cplusplus >= 202002L
        return this->write( 
            { .src_ptr = (char[]){ reg_, val_ }, .src_n = 2 }
        );
    #else
        char bytes[ 2 ] = { (char)reg_, (char)val_ };
        port_W_desc_t wd;
        wd.src_ptr = bytes;
        wd.src_n   = 2;
        return this->write( wd );
    #endif
    } 

};

};