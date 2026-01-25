/**
 * @file: osp/IO_sockets.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/IO_sockets.hpp>

namespace a113::io {

#define _CAGP _conn.addr_str.get(), _conn.port

A113_IMPL_FNC status_t IPv4_TCP_socket::bind_peer( ipv4_addr_t addr_, ipv4_port_t port_ ) {
    A113_ASSERT_OR( false ==_conn.alive.load( std::memory_order_acquire ) ) {
        A113_LOGE_IO_INT( 
            A113_ERR_WOULD_OVRWR, "Binding another peer whilst alive. [{}:{}] -> [{}:{}]",
             _CAGP, ipv4_addr_str_t::from( addr_ ).get(), port_ 
        );
        return -0x1;
    }

    _conn.sock     = NULL_SOCKET;
    _conn.addr_str = ipv4_addr_str_t::from( addr_ );
    _conn.addr     = addr_;
    _conn.port     = port_;

    A113_LOGI_IO( "Peer bound. [{}:{}]", _CAGP );
    return 0x0;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::bind_peer( const char* addr_str_, ipv4_port_t port_ ) {
    return this->bind_peer( ipv4_addr_str_t::from( addr_str_ ), port_ );
}

A113_IMPL_FNC status_t IPv4_TCP_socket::uplink( void ) {
    status_t status = 0x0;
    
    socket_t sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( sock >= 0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket descriptor on [{}:{}].", _CAGP );
        return sock;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void {
        closesocket( sock );
    } );

    sockaddr_in desc = {};
    memset( &desc, 0, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = _conn.addr;
    desc.sin_port        = _conn.port; 
    
    A113_LOGD_IO( "Uplinking to [{}:{}]...", _CAGP );
    status = ::connect( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) );
    A113_ASSERT_OR( 0x0 == status ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad uplink to [{}:{}].", _CAGP );
        return status;
    }

    _conn.sock     = sock;
    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI_IO( "Uplinked to [{}:{}].", _CAGP );
    A113_ON_SCOPE_EXIT_DROP;
    return status;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::downlink( void ) {
    status_t status = 0x0;

    _conn.alive.store( false, std::memory_order_seq_cst );
    
    status = ::closesocket( std::exchange( _conn.sock, NULL_SOCKET ) );
    A113_ASSERT_OR( 0x0 == status ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket closure on [{}:{}].", _CAGP );
    }

    _conn.addr = 0x0;
    _conn.addr_str.make_null(); 
    _conn.port = 0x0;

    return 0x0;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::listen( void ) {
    A113_ASSERT_OR( 0x0 == _conn.addr ) {
        A113_LOGE_IO_INT( A113_ERR_LOGIC, "Listening with peer address different from 0:0:0:0 -> [{}:{}].", _CAGP );
        return -0x1;
    }

    status_t status = 0x0;
    
    socket_t sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( sock >= 0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket descriptor on [{}:{}].", _CAGP );
        return sock;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void {
        ::closesocket( sock );
    } );

    sockaddr_in desc = {};
    memset( &desc, 0, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = 0x0;
    desc.sin_port        = _conn.port; 
    
    status = ::bind( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) );
    A113_ASSERT_OR( status == 0x0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket bind on [{}:{}].", _CAGP );
        return status;
    }

    A113_LOGI_IO( "Listening on [{}:{}]...", _CAGP ); 
    status = ::listen( sock, 1 );
    A113_ASSERT_OR( status == 0x0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket listen on [{}:{}].", _CAGP );
    }
    
    sockaddr_in in_desc = {}; 
    int         in_desc_sz = sizeof( sockaddr_in );
    memset( &in_desc, 0, sizeof( sockaddr_in ) );

    socket_t in_sock  = ::accept( sock, ( sockaddr* )&in_desc, &in_desc_sz );
    A113_ASSERT_OR( in_sock >= 0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket acceptance on [{}:{}].", _CAGP );
        return -0x1;
    }

    _conn.sock     = in_sock;
    _conn.addr     = in_desc.sin_addr.s_addr;
    _conn.addr_str = ipv4_addr_str_t::from( _conn.addr );
    _conn.port     = in_desc.sin_port;
    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI_IO( "Accepted [{}:{}].", _CAGP );

    return status;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::read( const port_R_desc_t& desc_ ) {
    return ::recv( _conn.sock, desc_.dst_ptr, desc_.dst_n, MSG_WAITALL );
}

A113_IMPL_FNC status_t IPv4_TCP_socket::write( const port_W_desc_t& desc_ ) {
    return ::send( _conn.sock, desc_.src_ptr, desc_.src_n, 0 );
}


}
