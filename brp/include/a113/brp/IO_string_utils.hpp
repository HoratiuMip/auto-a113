#pragma once
/**
 * @file: BRp/IO_string_utils.hpp
 * @brief: Utility function to convert between text and binary formns of IO-related types.
 * @details: The so dubbed "IO-related types" include: IPv4 addresses, Bluetooth adresses, and so forth.
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/brp/descriptor.hpp>
#include <a113/brp/IO_port.hpp>

namespace a113 { namespace io {

struct ipv4_addr_str_t {
    inline static constexpr int   BUF_SIZE   = 0x4*3 + 0x3 + 0x1;

    char   buf[ BUF_SIZE ]   = { '\0' }; 

    void make_zero( void ) { strcpy( buf, "0.0.0.0" ); }
    void make_null( void ) { buf[ 0 ] = { '\0' }; }

    static void from_ptr( ipv4_addr_t addr_, ipv4_addr_str_t* ptr_ );
    static ipv4_addr_str_t from( ipv4_addr_t addr_ );
    static ipv4_addr_t from( const char* addr_str_ );

    char* c_str( void ) { return buf; }
    operator char* ( void ) { return buf; }
};

struct ipv4_addr_pack_t : public ipv4_addr_str_t {
    ipv4_addr_pack_t( void ) = default;
    ipv4_addr_pack_t( ipv4_addr_t addr_ ) : ipv4_addr_str_t{ ipv4_addr_str_t::from( addr_ ) }, addr{ addr_ } {}
    ipv4_addr_pack_t( const char* addr_str_ ) : addr{ ipv4_addr_str_t::from( addr_str_ ) } {
        strncpy( ipv4_addr_str_t::buf, addr_str_, ipv4_addr_str_t::BUF_SIZE );
    }

    ipv4_addr_t   addr   = 0x0;
};

struct bt_addr_str_t {
    inline static constexpr int   BUF_SIZE   = 0x6*2 + 0x5 + 0x1;

    char   buf[ BUF_SIZE ]   = { '\0' };

    void make_zero( void ) { strcpy( buf, "00:00:00:00:00:00" ); }
    void make_null( void ) { buf[ 0 ] = { '\0' }; }

    static void from_ptr( bt_addr_t addr_, bt_addr_str_t* ptr_, char x_ = 'X' );
    static bt_addr_str_t from( bt_addr_t addr_, char x_ = 'X' );
    static bt_addr_t from( const bt_addr_str_t& addr_str_ );

    char* c_str( void ) { return buf; }
    operator char* ( void ) { return buf; }
};


struct bt_addr_pack_t : bt_addr_t, bt_addr_str_t {
    bt_addr_pack_t( void ) {}

    void pull_str( char x_ = 'X' ) { bt_addr_str_t::from_ptr( *this, this, x_ ); }
    void pull_n( void ) { static_cast< bt_addr_t& >( *this ) = bt_addr_str_t::from( static_cast< bt_addr_str_t& >( *this ) ); }
};

} };
