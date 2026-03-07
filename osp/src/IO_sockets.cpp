/**
 * @file: osp/IO_sockets.cpp
 * @brief: Implementation file.
 * @details: -
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/IO_sockets.hpp>

namespace a113::io {

#define _CAGP _conn.addr_str.str(), _conn.port

/* ===========================================
    Implementation for Windows.
*/
#ifdef A113_TARGET_OS_WINDOWS

A113_IMPL_FNC status_t IPv4_TCP_socket::bind( ipv4_addr_t addr_, ipv4_port_t port_ ) {
    A113_ASSERT_OR( false ==_conn.alive.load( std::memory_order_acquire ) ) {
        A113_LOGE_INT( 
            A113_ERR_WOULD_OVRWR, "Binding whilst alive {}:{} -> {}:{}.",
             _CAGP, ipv4_addr_str_t::from( addr_ ).str(), port_ 
        );
        return A113_ERR_WOULD_OVRWR;
    }

    _sock          = INVALID_SOCKET;
    _conn.addr_str = ipv4_addr_str_t::from( addr_ );
    _conn.addr     = addr_;
    _conn.port     = port_;

    A113_LOGI( "Bound {}:{}.", _CAGP );
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::bind( const char* addr_str_, ipv4_port_t port_ ) {
    return this->bind( ipv4_addr_str_t::from( addr_str_ ), port_ );
}

A113_IMPL_FNC status_t IPv4_TCP_socket::connect( const config_t& config_ ) {
    ::SOCKET sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( INVALID_SOCKET != sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket for {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void { ::closesocket( sock ); } );

    sockaddr_in desc = {};
    ZeroMemory( &desc, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = _conn.addr;
    desc.sin_port        = htons( _conn.port ); 
    
    A113_LOGI( "Connecting {}:{}...", _CAGP );
    A113_ASSERT_OR( 0x0 == ::connect( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad connect {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    _sock = sock;

    if( 0 != config_.timeouts.outbound_s && 0 != config_.timeouts.inbound_s )
        this->timeouts( config_.timeouts );

    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI( "Connected {}:{}.", _CAGP );
    A113_ON_SCOPE_EXIT_DROP;
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::disconnect( void ) {
    _conn.alive.store( false, std::memory_order_seq_cst );
    
    if( INVALID_SOCKET == _sock ) return A113_OK;
    
    A113_ASSERT_OR( 0x0 == ::closesocket( std::exchange( _sock, INVALID_SOCKET ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket close {}:{}.", _CAGP );
    }

    A113_LOGI( "Disconnected {}:{}.", _CAGP );
    _conn.addr = 0x0;
    _conn.addr_str.make_null(); 
    _conn.port = 0x0;

    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::listen( void ) {
    A113_ASSERT_OR( 0x0 == _conn.addr ) {
        A113_LOGE_INT( A113_ERR_LOGIC, "Listening must be on 0:0:0:0 ({}:{}).", _CAGP ); return A113_ERR_LOGIC;
    }

    ::SOCKET sock = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( INVALID_SOCKET != sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void { ::closesocket( sock ); } );

    sockaddr_in desc = {};
    ZeroMemory( &desc, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = 0x0;
    desc.sin_port        = htons( _conn.port ); 
    
    A113_ASSERT_OR( 0x0 == ::bind( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad bind {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ASSERT_OR( 0x0 == ::listen( sock, 1 ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad listen {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    _sock = sock; A113_ON_SCOPE_EXIT_DROP;

    A113_LOGI( "Listening on {}:{}...", _CAGP ); 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::accept( IPv4_TCP_socket* sock_, const config_t& config_ ) {
    sockaddr_in in_desc    = {}; 
    int         in_desc_sz = sizeof( sockaddr_in );

    ZeroMemory( &in_desc, sizeof( sockaddr_in ) );

    ::SOCKET in_sock = ::accept( _sock, ( sockaddr* )&in_desc, &in_desc_sz );
    A113_ASSERT_OR( INVALID_SOCKET != in_sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad accept {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    sock_->_sock = in_sock;

    if( 0 != config_.timeouts.outbound_s && 0 != config_.timeouts.inbound_s )
        sock_->timeouts( config_.timeouts );

    sock_->_conn.addr     = in_desc.sin_addr.s_addr;
    sock_->_conn.addr_str = ipv4_addr_str_t::from( sock_->_conn.addr );
    sock_->_conn.port     = in_desc.sin_port;
    sock_->_conn.alive.store( true, std::memory_order_release );

    A113_LOGI( "Accepted {}:{}.", sock_->addr_c_str(), sock_->port() );
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::read( const port_R_desc_t& desc_ ) {
    auto byte_count = ::recv( _sock, desc_.dst_ptr, desc_.dst_n, desc_.req_all ? MSG_WAITALL : 0x0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.req_all && byte_count != desc_.dst_n ) return A113_ERR_FLOW;
    if( desc_.reset && 0x0 == byte_count ) *desc_.reset = true; 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::write( const port_W_desc_t& desc_ ) {
    auto byte_count = ::send( _sock, desc_.src_ptr, desc_.src_n, 0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.req_all && byte_count != desc_.src_n ) return A113_ERR_FLOW;
    if( desc_.reset && 0x0 == byte_count ) *desc_.reset = true; 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::timeouts( const timeouts_t& tos_ ) {
    const DWORD rcvtimeo = ( DWORD )tos_.inbound_s * 1000;
    A113_ASSERT_OR( 0x0 == setsockopt( _sock, SOL_SOCKET, SO_RCVTIMEO, ( const char* )&rcvtimeo, sizeof( rcvtimeo ) ) ) {
        A113_LOGW_EX( A113_ERR_SYSCALL, "Bad set inbound timeout." );
    }

    const DWORD sndtimeo = ( DWORD )tos_.outbound_s * 1000;
    A113_ASSERT_OR( 0x0 == setsockopt( _sock, SOL_SOCKET, SO_RCVTIMEO, ( const char* )&sndtimeo, sizeof( sndtimeo ) ) ) {
        A113_LOGW_EX( A113_ERR_SYSCALL, "Bad set outbound timeout." );
    }

    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::holding_rx( int* bc_ ) {
    *bc_ = -0x1;

    ULONG bc = 0;
    A113_ASSERT_OR( 0x0 == ioctlsocket( _sock, FIONREAD, &bc ) ) return A113_ERR_SYSCALL;

    *bc_ = (int)bc;
    return A113_OK;
}

/* ===========================================
    Implementation for Linux.
*/
#elifdef A113_TARGET_OS_LINUX

A113_IMPL_FNC status_t IPv4_TCP_socket::bind( ipv4_addr_t addr_, ipv4_port_t port_ ) {
    A113_ASSERT_OR( false ==_conn.alive.load( std::memory_order_acquire ) ) {
        A113_LOGE_INT( 
            A113_ERR_WOULD_OVRWR, "Binding whilst alive {}:{} -> {}:{}.",
             _CAGP, ipv4_addr_str_t::from( addr_ ).str(), port_ 
        );
        return A113_ERR_WOULD_OVRWR;
    }

    _sock          = -0x1;
    _conn.addr_str = ipv4_addr_str_t::from( addr_ );
    _conn.addr     = addr_;
    _conn.port     = port_;

    A113_LOGI( "Bound {}:{}.", _CAGP );
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::bind( const char* addr_str_, ipv4_port_t port_ ) {
    return this->bind( ipv4_addr_str_t::from( addr_str_ ), port_ );
}

A113_IMPL_FNC status_t IPv4_TCP_socket::connect( const config_t& config_ ) {
    int sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( 0x0 < sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket for {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void { ::close( sock ); } );

    sockaddr_in desc = {};
    memset( &desc, 0, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = _conn.addr;
    desc.sin_port        = htons( _conn.port ); 
    
    A113_LOGI( "Connecting {}:{}...", _CAGP );
    A113_ASSERT_OR( 0x0 == ::connect( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad connect {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    _sock = sock;

    if( 0 != config_.timeouts.outbound_s && 0 != config_.timeouts.inbound_s )
        this->timeouts( config_.timeouts );

    _conn.alive.store( true, std::memory_order_release );

    A113_LOGI( "Connected {}:{}.", _CAGP );
    A113_ON_SCOPE_EXIT_DROP;
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::disconnect( void ) {
    _conn.alive.store( false, std::memory_order_seq_cst );

    if( -0x1 == _sock ) return A113_OK;
    
    A113_ASSERT_OR( 0x0 == ::close( std::exchange( _sock, -0x1 ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket close {}:{}.", _CAGP );
    }

    A113_LOGI( "Disconnected {}:{}.", _CAGP );
    _conn.addr = 0x0;
    _conn.addr_str.make_null(); 
    _conn.port = 0x0;

    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::listen( void ) {
    A113_ASSERT_OR( 0x0 == _conn.addr ) {
        A113_LOGE_INT( A113_ERR_LOGIC, "Listening must be on 0:0:0:0 ({}:{}).", _CAGP ); return A113_ERR_LOGIC;
    }

    int sock = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    A113_ASSERT_OR( 0x0 < sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad socket {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ON_SCOPE_EXIT_L( [ &sock ] -> void { ::close( sock ); } );

    sockaddr_in desc = {};
    memset( &desc, 0, sizeof( sockaddr_in ) );

    desc.sin_family      = AF_INET;
    desc.sin_addr.s_addr = 0x0;
    desc.sin_port        = htons( _conn.port ); 
    
    A113_ASSERT_OR( 0x0 == ::bind( sock, ( sockaddr* )&desc, sizeof( sockaddr_in ) ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad bind {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    A113_ASSERT_OR( 0x0 == ::listen( sock, 1 ) ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad listen {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }
    
    _sock = sock; A113_ON_SCOPE_EXIT_DROP;

    A113_LOGI( "Listening on {}:{}.", _CAGP ); 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::accept( IPv4_TCP_socket* sock_, const config_t& config_ ) {
    sockaddr_in  in_desc    = {}; 
    unsigned int in_desc_sz = sizeof( sockaddr_in );

    memset( &in_desc, 0, in_desc_sz );

    int in_sock = ::accept( _sock, ( sockaddr* )&in_desc, &in_desc_sz );
    A113_ASSERT_OR( 0x0 < in_sock ) {
        A113_LOGE_EX( A113_ERR_SYSCALL, "Bad accept {}:{}.", _CAGP ); return A113_ERR_SYSCALL;
    }

    sock_->_sock = in_sock;

    if( 0 != config_.timeouts.outbound_s && 0 != config_.timeouts.inbound_s )
        sock_->timeouts( config_.timeouts );

    sock_->_conn.addr     = in_desc.sin_addr.s_addr;
    sock_->_conn.addr_str = ipv4_addr_str_t::from( sock_->_conn.addr );
    sock_->_conn.port     = in_desc.sin_port;
    sock_->_conn.alive.store( true, std::memory_order_release );

    A113_LOGI( "Accepted {}:{}.", sock_->addr_c_str(), sock_->port() );
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::read( const port_R_desc_t& desc_ ) {
    auto byte_count = ::recv( _sock, desc_.dst_ptr, desc_.dst_n, desc_.req_all ? MSG_WAITALL : 0x0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.req_all && byte_count != desc_.dst_n ) return A113_ERR_FLOW;
    if( desc_.reset && 0x0 == byte_count ) *desc_.reset = true; 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::write( const port_W_desc_t& desc_ ) {
    auto byte_count = ::send( _sock, desc_.src_ptr, desc_.src_n, 0 );
    if( desc_.byte_count ) *desc_.byte_count = byte_count;
    if( desc_.req_all && byte_count != desc_.src_n ) return A113_ERR_FLOW;
    if( desc_.reset && 0x0 == byte_count ) *desc_.reset = true; 
    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::timeouts( const timeouts_t& tos_ ) {
    const timeval rcvtimeo = {
        .tv_sec  = tos_.inbound_s,
        .tv_usec = 0
    };
    A113_ASSERT_OR( 0x0 == setsockopt( _sock, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo, sizeof( rcvtimeo ) ) ) {
        A113_LOGW_EX( A113_ERR_SYSCALL, "Bad set inbound timeout." );
    }

    const timeval sndtimeo = {
        .tv_sec  = tos_.outbound_s,
        .tv_usec = 0
    };
    A113_ASSERT_OR( 0x0 == setsockopt( _sock, SOL_SOCKET, SO_RCVTIMEO, &sndtimeo, sizeof( sndtimeo ) ) ) {
        A113_LOGW_EX( A113_ERR_SYSCALL, "Bad set outbound timeout." );
    }

    return A113_OK;
}

A113_IMPL_FNC status_t IPv4_TCP_socket::holding_rx( int* bc_ ) {
    *bc_ = -0x1;

    A113_ASSERT_OR( 0x0 == ioctl( _sock, FIONREAD, bc_ ) ) return A113_ERR_SYSCALL;
    return A113_OK;
}

#else
    #error "[A113]: Building sockets with no or unsupported target operating system."
#endif

}
