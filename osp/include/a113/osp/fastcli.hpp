#pragma once
/**
 * @file: osp/fastcli.hpp
 * @brief: 
 * @details
 * @authors: Vatca "Mipsan" Tudor-Horatiu
 */

#include <a113/osp/core.hpp>

namespace a113::text {

class Fastcli {
public:
    struct config_t {
        std::string   xtra_chrs   = " ._/";
    };

    using cmd_fnc_t = std::function< status_t( const std::string& cmd_, void* ctx_ ) >;
    
    struct cmd_t {
        std::string   text;
        cmd_fnc_t     fnc;
        int           min_args;
    };

    using cmd_map_t = std::map< std::string, cmd_fnc_t >;

public:
    Fastcli( void ) = default;

    Fastcli( const config_t& config_, const cmd_map_t& cmd_map_ ) 
    : _config{ config_ }, _cmd_map{ cmd_map_ }
    {}

_A113_PROTECTED:
    config_t            _config        = {};
    cmd_map_t           _cmd_map       = {};
    std::shared_mutex   _cmd_map_mtx   = {};

public:
    status_t parse( const std::string& cmd_, std::string* out_ ) {
        std::vector< std::string > toks; toks.reserve( 0x4 );
        
        size_t lti = 0x0;
        size_t rti = 0x0;
        bool   iq  = false;

        do {
            lti = cmd_.find_first_not_of( 0x20, rti );
            A113_ASSERT_OR( lti != std::string::npos ) {
                if( rti != 0x0 ) break;
                else {
                    *out_ = "Ill-formed command line."; return A113_ERR_BADARG;
                }
            }

            switch( cmd_[ lti ] ) {
                case '\"': 
            }

            rti = cmd_.find_first_of( 0x20, lti );
            
            auto& tok = toks.emplace_back( cmd_.substr( lti, rti - lti ) );
            for( auto c : tok ) {
                A113_ASSERT_OR(
                    std::isalpha( c ) || std::isdigit( c )
                    ||
                    _config.xtra_chrs.find( c ) != std::string::npos
                ) {
                    *out_ = "Invalid characters found."; return A113_ERR_BADARG;
                }
            }
        } while( rti < cmd_.length() );

        
    }

};

}