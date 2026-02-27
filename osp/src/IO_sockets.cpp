/**
 * @file: osp/IO_sockets.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/IO_sockets.hpp>

namespace a113::io {

#define _CAGP _conn.addr_str.str(), _conn.port

A113_IMPL_FNC status_t IPv4_TCP_socket::bind_peer( ipv4_addr_t addr_, ipv4_port_t port_ ) {
    A113_ASSERT_OR( false ==_conn.alive.load( std::memory_order_acquire ) ) {
        A113_LOGE_IO_INT( 
            A113_ERR_WOULD_OVRWR, "Binding another peer whilst alive. [{}:{}] -> [{}:{}]",
             _CAGP, ipv4_addr_str_t::from( addr_ ).str(), port_ 
        );
        return -0x1;
    }

    _conn.sock     = INVALID_SOCKET;
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
    #ifdef A113_TARGET_OS_WINDOWS
        ::closesocket( sock );
    #elifdef A113_TARGET_OS_LINUX
        ::close( sock );
    #endif
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

    _conn.sock = sock;
    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI_IO( "Uplinked to [{}:{}].", _CAGP );
    A113_ON_SCOPE_EXIT_DROP;
    return status;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::downlink( void ) {
    status_t status = 0x0;

    _conn.alive.store( false, std::memory_order_seq_cst );
    
#ifdef A113_TARGET_OS_WINDOWS
    ::closesocket( std::exchange( _conn.sock, INVALID_SOCKET ) );
#elifdef A113_TARGET_OS_LINUX
    ::close( std::exchange( _conn.sock, INVALID_SOCKET ) );
#endif
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
    #ifdef A113_TARGET_OS_WINDOWS
        ::closesocket( sock );
    #elifdef A113_TARGET_OS_LINUX
        ::close( sock );
    #endif
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
        return A113_ERR_SYSCALL;
    }
    
    sockaddr_in in_desc    = {}; 
#ifdef A113_TARGET_OS_LINUX
    unsigned
#endif
    int         in_desc_sz = sizeof( sockaddr_in );
    memset( &in_desc, 0, sizeof( sockaddr_in ) );

    socket_t in_sock  = ::accept( sock, ( sockaddr* )&in_desc, &in_desc_sz );
    A113_ASSERT_OR( in_sock >= 0 ) {
        A113_LOGE_IO_EX( A113_ERR_SYSCALL, "Bad socket accept on [{}:{}].", _CAGP );
        return A113_ERR_SYSCALL;
    }

    _conn.sock     = in_sock;
    _conn.addr     = in_desc.sin_addr.s_addr;
    _conn.addr_str = ipv4_addr_str_t::from( _conn.addr );
    _conn.port     = in_desc.sin_port;
    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI_IO( "Accepted [{}:{}].", _CAGP );
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::read( const port_R_desc_t& desc_ ) {
    auto byte_count = ::recv( _conn.sock, desc_.dst_ptr, desc_.dst_n, desc_.fail_if_not_all ? MSG_WAITALL : 0x0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.fail_if_not_all && byte_count != desc_.dst_n ) return A113_ERR_FLOW;
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::write( const port_W_desc_t& desc_ ) {
    auto byte_count = ::send( _conn.sock, desc_.src_ptr, desc_.src_n, 0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.fail_if_not_all && byte_count != desc_.src_n ) return A113_ERR_FLOW;
    return A113_OK;
}


}
