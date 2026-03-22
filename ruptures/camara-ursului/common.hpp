#pragma once
// ======================= Includes =======================
#include <a113/gep/text_utils.hpp>
#include <a113/gep/fastcli.hpp>
#include <a113/osp/IO_sockets.hpp>

#include <nlohmann/json.hpp>

// ======================= Defines =======================
#define   CU_ASSERT_OR(c) A113_ASSERT_OR(c)

// ======================= Config =======================
#define   DEFAULT_PORT                        58008

#define   DEFAULT_SERVER_INBOUND_TIMEOUT_S    15
#define   DEFAULT_SERVER_OUTBOUND_TIMEOUT_S   15

/* Time before an unsubscribed client is terminated due to inactivity. */
#define   CU_DEFAULT_SERVER_UNSUBS_HOLD_TIME_S         300
/* Time before a pantry is terminated due to lack of clients. */
#define   CU_DEFAULT_SERVER_PANTRY_IDLE_ALLOW_TIME_S   30

#define   CU_DEFAULT_CLIENT_INBOUND_TIMEOUT_S    15
#define   CU_DEFAULT_CLIENT_OUTBOUND_TIMEOUT_S   15

#define   CU_MAX_PACKET_SIZE                     1024
#define   CU_SERVER_DROP_CLIENT_AFTER_FAIL_N     3

// ======================= Utility =======================
using namespace std; using namespace a113;

inline status_t CU_respond( io::IPv4_TCP_socket& sock_, const string& resp_ ) {
    return sock_.write( {
        .src_ptr = const_cast< string& >( resp_ ).data(),
        .src_n   = (int)resp_.length()
    } );
}

status_t CU_request( io::IPv4_TCP_socket& sock_, const string& req_, nlohmann::json* resp_ ) {
    CU_ASSERT_OR( A113_OK == sock_.write( {
        .src_ptr = const_cast< string& >( req_ ).data(),
        .src_n   = (int)req_.length()
    } ) ) return A113_ERR_ENGINECALL;

    char buffer[ CU_MAX_PACKET_SIZE ];
    int  byte_count = 0x0;

    CU_ASSERT_OR( A113_OK == sock_.read( {
        .dst_ptr    = buffer,
        .dst_n      = CU_MAX_PACKET_SIZE,
        .byte_count = &byte_count
    } ) && byte_count > 0x0 ) return A113_ERR_ENGINECALL;

    try {
        *resp_ = nlohmann::json::parse( buffer, buffer + byte_count );
    } catch( ... ) {
        return A113_ERR_BADARG;
    }
    return A113_OK;
}
