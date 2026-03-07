#pragma once

// ======================= Config =======================
#define   DEFAULT_PORT                        58008

#define   DEFAULT_SERVER_INBOUND_TIMEOUT_S    15
#define   DEFAULT_SERVER_OUTBOUND_TIMEOUT_S   15
#define   DEFAULT_SERVER_UNSUBS_HOLD_TIME_S   60

#define   MAX_PACKET_SIZE                     1024
#define   SERVER_DROP_UNSUB_AFTER_FAIL_N      3

#define   REQ_SPLT_CHR                        '$'

#define   OP_JOIN_ROOM                        0x0A

// ======================= Utility =======================
using namespace std;

static constexpr uint32_t hash_unsecure( const string& str_ ) {
    uint32_t h = 2166136261U;
    for( char c : str_ ) {
        h ^= (uint32_t)c;
        h *= 16777619U;
    }
    return h;
}