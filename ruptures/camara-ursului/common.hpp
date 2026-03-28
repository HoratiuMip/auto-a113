#pragma once
// ======================= Includes =======================
#include <a113/gep/text_utils.hpp>
#include <a113/gep/fastcli.hpp>
#include <a113/osp/IO_sockets.hpp>

#include <nlohmann/json.hpp>

// ======================= Defines =======================
#define   CU_ASSERT_OR(c) A113_ASSERT_OR(c)

#define   CU_PROTOCOL_ASSERT(c,m) CU_ASSERT_OR(c) throw runtime_error{(m)}

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
#define   CU_DEFAULT_CLIENT_REQ_TIMEOUT_S        15

#define   CU_MAX_PACKET_SIZE                     1024
#define   CU_SERVER_DROP_CLIENT_AFTER_FAIL_N     3


// ======================= Utility =======================
using namespace std; using namespace a113;

inline status_t CU_respond( io::IPv4_TCP_socket& sock_, const nlohmann::json& json_ ) {
    auto dump = json_.dump();
    return sock_.write( {
        .src_ptr = dump.data(),
        .src_n   = (int)dump.length()
    } );
}

inline status_t CU_request( io::IPv4_TCP_socket& sock_, const nlohmann::json& json_ ) {
    return CU_respond( sock_, json_ ); 
}

inline status_t CU_await( io::IPv4_TCP_socket& sock_, nlohmann::json* json_ ) {
    char     buffer[ CU_MAX_PACKET_SIZE ];
    int      byte_count = 0;
    status_t status     = A113_OK;

    CU_ASSERT_OR( A113_OK == ( status = sock_.read( {
        .dst_ptr    = buffer,
        .dst_n      = CU_MAX_PACKET_SIZE,
        .byte_count = &byte_count,
        .req_all    = false,
        .req_time   = true
    } ) ) ) return status;

    CU_ASSERT_OR( byte_count > 0 ) return A113_ERR_FLOW;

    *json_ = nlohmann::json::parse( buffer, buffer + byte_count );
    return A113_OK;
}

