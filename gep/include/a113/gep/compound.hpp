#pragma once
/**
 * @file: gep/compound.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */
#include <a113/gep/core.hpp>
#include <a113/gep/dispenser.hpp>

namespace a113 {

class Compound {
public:
    enum State_ {
        State_STOPPED, State_STARTED, State_STOPPING, State_STARTING
    };

_A113_PROTECTED:
    std::atomic< State_ >   _compound_state   = { State_STOPPED };

_A113_PROTECTED:
    virtual status_t _compound_start( void* ctx_ ) = 0;

    virtual status_t _compound_stop( void* ctx_ ) = 0;

public:
    virtual std::string_view compound_name( void ) const = 0;

public:
    status_t compound_start( void* ctxu_ = nullptr, void* ctxud_ = nullptr ) {
        State_ es = State_STOPPED;
        A113_ASSERT_OR( _compound_state.compare_exchange_strong( es, State_STARTING, std::memory_order_release ) ) {
            return A113_ERR_WOULD_OVRWR;
        }
      
        status_t status = this->_compound_start( ctxu_ );
        A113_ASSERT_OR( A113_OK == status ) {
            this->_compound_stop( ctxud_ );
            _compound_state.store( State_STOPPED, std::memory_order_release );
            return status;
        }
        _compound_state.store( State_STARTED, std::memory_order_release );
        return A113_OK;
    }

    status_t compound_stop( void* ctxd_ = nullptr ) {
        State_ es = State_STARTED;
        A113_ASSERT_OR( _compound_state.compare_exchange_strong( es, State_STOPPING, std::memory_order_release ) ) {
            return A113_ERR_WOULD_OVRWR;
        }

        status_t status = this->_compound_stop( ctxd_ );
        A113_ASSERT_OR( A113_OK == status ) {
            _compound_state.store( State_STOPPED, std::memory_order_release );
            return status;
        }
        _compound_state.store( State_STOPPED, std::memory_order_release );
        return A113_OK;
    }

    virtual status_t compound_restart( void* ctxd_ = nullptr, void* ctxu_ = nullptr, void* ctxud_ = nullptr ) {
        this->compound_stop( ctxd_ );
        return this->compound_start( ctxu_, ctxud_ );
    }

public:
    A113_inline bool compound_is_up( void ) {
        return State_STARTED == _compound_state.load( std::memory_order_relaxed );
    }

    A113_inline bool compound_is_stable( void ) {
        State_ state = _compound_state.load( std::memory_order_relaxed );
        return state != State_STOPPING and state != State_STARTING;
    }
};

class CompoundCluster {
public:
    typedef   std::function< void( HVec< Compound > ) >   critical_fnc_t;

public:
    struct restart_if_args_t {
        int   attempt   = 0;
    };

    struct entry_t {
        typedef   std::function< status_t( Compound&, const restart_if_args_t& ) >   restart_if_fnc_t;

        HVec< Compound >                  ref                   = nullptr;
        std::vector< HVec< Compound > >   deps                  = {};
        bool                              keep_alive            = false;
        restart_if_fnc_t                  restart_if            = nullptr;
        int                               critical_n_restarts   = -1;
        void*                             ctxu                  = nullptr;
        void*                             ctxd                  = nullptr;
        void*                             ctxud                 = nullptr;

        mutable int                       _failed_restart_cnt   = 0;
    };

_A113_PROTECTED:
    struct _entry_compare_t {
        bool operator () ( const entry_t& lhs_, const entry_t& rhs_ ) const { 
            return lhs_.ref->compound_name() < rhs_.ref->compound_name();
        }
    };

_A113_PROTECTED:
    Dispenser< std::set< entry_t, _entry_compare_t > >  _register    = { DispenserMode_Lock };

    std::atomic_bool                                    _crit_flag   = { false };
    critical_fnc_t                                      _crit_fnc    = {};

public:
    void when_critical( const critical_fnc_t& crit_fnc_ ) { _crit_fnc = crit_fnc_; }

    void go_critical( HVec< Compound > cmpd_ ) {
        bool exflag = false;
        A113_ASSERT_OR( _crit_flag.compare_exchange_strong( exflag, true, std::memory_order_seq_cst ) ) return;
        if( _crit_fnc ) this->_crit_fnc( std::move( cmpd_ ) );
    }

    A113_inline bool went_critical( void ) { return _crit_flag.load( std::memory_order_relaxed ); }

public:
    status_t push( entry_t entry_ ) {
        auto reg = _register.control();
        auto [ itr, inserted ] = reg->emplace( std::move( entry_ ) );
        return inserted ? A113_OK : A113_ERR_WOULD_OVRWR;
    }

    status_t pop( std::string_view name_ ) {
        auto reg = _register.control();
        return std::erase_if( *reg, [ &name_ ] ( const entry_t& entry_ ) -> bool {
            return name_ == entry_.ref->compound_name();
        } ) > 0 ? A113_OK : A113_ERR_NOT_FOUND;
    }

public:
    status_t iterate_register( void ) {
        auto reg = _register.watch();

        for( auto& entry : *reg ) {
            A113_ASSERT_OR( entry.ref->compound_is_stable() ) continue;

            int up_deps = 0;
            for( auto& dep : entry.deps ) up_deps += dep->compound_is_up();
            A113_ASSERT_OR( up_deps == entry.deps.size() ) continue;
            
            const bool needs_restart = 
                ( entry.keep_alive && not entry.ref->compound_is_up() )
                ||
                ( A113_OK != entry.restart_if( *entry.ref, {
                    .attempt = entry._failed_restart_cnt
                } ) );

            if( needs_restart ) {
                A113_ASSERT_OR( A113_OK == entry.ref->compound_restart( entry.ctxd, entry.ctxu, entry.ctxud ) ) {
                    ++entry._failed_restart_cnt;

                    if( entry.critical_n_restarts != -1 && entry._failed_restart_cnt >= entry.critical_n_restarts ) {
                        this->go_critical( *entry.ref );
                        return A113_ERR_TERMINATED; 
                    }
                } else {
                    entry._failed_restart_cnt = 0;
                }
            }
        }
        return A113_OK;
    }

};

};