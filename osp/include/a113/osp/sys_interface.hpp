#pragma once
/**
 * @file: osp/sys_interface.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>

namespace a113::sys {

class NTHInvokes {
public:
    NTHInvokes( void ) : _th_main{ std::thread{ &NTHInvokes::_main, this } } {}

    ~NTHInvokes( void ) {
        _online.store( false, std::memory_order_release );

        _q.length.store( -0x1, std::memory_order_release );
        _q.length.notify_one();

         if( _th_main.joinable() ) _th_main.join();
    }

_A113_PROTECTED:
    enum _InvokeMsg_ : int {
        _InvokeMsg_None,
        _InvokeMsg_PushDesktopNotification
    };

    typedef   std::pair< _InvokeMsg_, std::any >   _pack_t;

    struct _push_desktop_notification_t {
        std::string   who       = {};
        std::string   title     = {};
        std::string   detail    = {};
        int           hold_ms   = 3'000;
    };

_A113_PROTECTED:
    std::thread        _th_main   = {};
    std::atomic_bool   _online    = { false };

    struct _queue_t {
        std::queue< std::pair< _InvokeMsg_, std::any > >   ueue;
        std::atomic_int                                    length   = { 0 };
        std::mutex                                         mtx;
    } _q;

_A113_PROTECTED:
    void _main( void ) {
        _online.store( true, std::memory_order_release );

    for(;_online.load( std::memory_order_relaxed);) {
        _q.length.wait( 0x0 );
        A113_ASSERT_OR( _online.load( std::memory_order_acquire ) ) break;

        _pack_t pack;
    {
        std::lock_guard lck{ _q.mtx };
        pack = std::move( _q.ueue.front() );
        _q.ueue.pop();
        _q.length.fetch_sub( 1, std::memory_order_release );
    }
    #define _GET_ARGS( t ) auto& args = std::any_cast< t& >( pack.second )
        switch( pack.first ) {
            case _InvokeMsg_PushDesktopNotification: { _GET_ARGS( _push_desktop_notification_t );
                NOTIFYICONDATA nid = {
                    .cbSize = sizeof( NOTIFYICONDATA ),
                    .hWnd   = FindWindowA( nullptr, args.who.c_str() ),
                    .uID    = 1,
                    .uFlags = NIF_INFO | NIF_MESSAGE
                };

                lstrcpy( nid.szInfoTitle, args.title.c_str() );
                lstrcpy( nid.szInfo, args.detail.c_str() );

                Shell_NotifyIcon( NIM_ADD, &nid );
                std::this_thread::sleep_for( std::chrono::milliseconds{ args.hold_ms } );
                Shell_NotifyIcon( NIM_DELETE, &nid );
            break; }
        }
    #undef _GET_ARGS
    } }

    void _dispatch( _pack_t&& pack_ ) {
    {
        std::lock_guard lck{ _q.mtx };
        _q.ueue.push( std::move( pack_ ) ); 
    }
        _q.length.fetch_add( 1, std::memory_order_release );
        _q.length.notify_one();
    }

public:
    status_t push_desktop_notification( _push_desktop_notification_t&& args_ ) {
        this->_dispatch( { _InvokeMsg_PushDesktopNotification, std::move( args_ ) } ); return A113_OK;
    }

};

class _NTHInvokes_Win : public NTHInvokes {
public:

};

};
