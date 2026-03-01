#pragma once
/**
 * @file: osp/IO_sockets.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>

#include <a113/brp/IO_port.hpp>
#include <a113/brp/IO_string_utils.hpp>


namespace a113::io { 

class IPv4_TCP_socket : public Port {
#ifdef A113_TARGET_OS_WINDOWS

_A113_PROTECTED:
    ::SOCKET   _sock   = INVALID_SOCKET;

#elifdef A113_TARGET_OS_LINUX

_A113_PROTECTED:
    int   _sock   = -0x1;

#endif

public:
    struct timeouts_t {
        int   outbound_ms;
        int   inbound_ms;
        int   tcp_ms;
    };

_A113_PROTECTED:
    struct _conn_t {
        std::atomic_bool   alive      = { false };
        ipv4_addr_str_t    addr_str   = {};
        ipv4_addr_t        addr       = 0x0;
        ipv4_port_t        port       = 0x0;
    } _conn;

public:
    A113_inline const char* addr_c_str( void ) { return _conn.addr_str.buf; }
    A113_inline ipv4_port_t port( void ) { return _conn.port; }

public:
    status_t bind( ipv4_addr_t addr_, ipv4_port_t port_ );
    status_t bind( const char* addr_str_, ipv4_port_t port_ );

public:
    status_t connect( void );
    status_t disconnect( void );

    status_t listen( void );

public:
    virtual status_t read( const port_R_desc_t& desc_ ) override;
    virtual status_t write( const port_W_desc_t& desc_ ) override;

public:
    status_t timeouts( const timeouts_t& tos_ );

};

}



