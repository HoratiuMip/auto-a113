#pragma once
/**
 * @file: brp/IO_port.hpp
 * @brief: Basic input/output interface.
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/descriptor.hpp>

namespace a113 { namespace io {

typedef   uint32_t   ipv4_addr_t;
typedef   uint16_t   ipv4_port_t;

typedef   struct bt_addr_t { uint8_t b[6]; }   bt_addr_t;

typedef   uint8_t    i2c_addr_t;

struct port_R_desc_t {
    char*     dst_ptr           = nullptr;
    size_t    dst_n             = 0;
    size_t*   byte_count        = nullptr;
    bool      fail_if_not_all   = false;
};
struct port_W_desc_t {
    char*     src_ptr           = nullptr;
    size_t    src_n             = 0;
    size_t*   byte_count        = nullptr;
    bool      fail_if_not_all   = false;
};

class Port {
public:
    virtual status_t read( const port_R_desc_t& desc_ ) = 0;

    virtual status_t write( const port_W_desc_t& desc_ ) = 0;

public:
    status_t basic_read_loop( const port_R_desc_t& desc_ );

    status_t basic_write_loop( const port_W_desc_t& desc_ );
};


} };
