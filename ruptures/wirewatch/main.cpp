#include <a113/brp/IO_string_utils.hpp>
#include <a113/osp/core.hpp>

#include <pcap.h>
#include <ethertype.h>

#include <netinet/udp.h>

#define WW_ASSERT_OR(c) A113_ASSERT_OR(c)

struct catch_t {
    enum type_e {
        unknown, raw, tzsp
    };
    inline static const char* const type_s[] = {
        "unknown", "raw", "tzsp"
    };

    time_t   timestamp   = 0x0;
    type_e   type        = unknown;
    iphdr    ip_hdr      = {};

    struct _src_t : a113::io::ipv4_addr_pack_t { 
        a113::io::ipv4_addr_pack_t   addr;     
    } src;

    struct _dst_t : a113::io::ipv4_addr_pack_t {
        a113::io::ipv4_addr_pack_t   addr;  
    } dst;
};

std::map< std::string, catch_t >   map;

void resolve_catch( catch_t ctch_ ) {
    map[ std::format( "{:15} {:15}", ctch_.src.addr.c_str(), ctch_.dst.addr.c_str() ) ] = std::move( ctch_ );

    auto t_now = time( nullptr );

    std::string out = "\n";

    for( auto itr = map.begin(); itr != map.end(); ) {
        auto& [ key, ctch ] = *itr;

        if( t_now - ctch.timestamp < 5 ) {
            const auto* proto = getprotobynumber( ctch.ip_hdr.protocol );
            
            out += std::format( "{} | {:5} | {:6}\n", key, catch_t::type_s[ ctch.type ], proto ? proto->p_name : "" );
            ++itr;
        } else {
            itr = map.erase( itr );
        }
    }
    spdlog::info( "{}", out );
}

void resolve_packet( catch_t&& ctch_, const iphdr* ip_hdr_ ) {
    ctch_.src.addr = a113::io::ipv4_addr_pack_t{ ip_hdr_->saddr };
    ctch_.dst.addr = a113::io::ipv4_addr_pack_t{ ip_hdr_->daddr };
    resolve_catch( ctch_ );
}

void packet_handler( u_char* args_, const pcap_pkthdr* hdr_, const u_char* pck_ ) {
    catch_t ctch = {
        .timestamp = hdr_->ts.tv_sec
    };

    const u_char* ptr  = pck_ + offsetof( ether_header, ether_type );
    const u_char* end  = pck_ + hdr_->caplen;

#define _PTR_MOVE(n,m) WW_ASSERT_OR( (ptr += (n)) < end && ( end - ptr ) >= (m)) { spdlog::error( "parsing pointer overflow." ); return; }

l_dive_ether_type:
    switch( ntohs( *( uint16_t* )ptr ) ) {
        case ETHERTYPE_IP: {
            _PTR_MOVE( 2, sizeof( iphdr ) );

            auto* ip_hdr = ( const iphdr* )ptr;
            
            ctch.ip_hdr = *ip_hdr;

            if( ip_hdr->protocol != IPPROTO_UDP ) {
                ctch.type = catch_t::raw;
                resolve_packet( std::move( ctch ), ip_hdr );
            }

            _PTR_MOVE( ip_hdr->ihl * 4, sizeof( udphdr ) );
            auto* udp_hdr = ( const udphdr* )ptr;

            if( ntohs( udp_hdr->uh_dport ) != 37008 ) {
                ctch.type = catch_t::raw;
                resolve_packet( std::move( ctch ), ip_hdr );
                return;
            }
            
            _PTR_MOVE( sizeof( *udp_hdr ), 0 );
            int tzsp_len = hdr_->caplen - (ptr - pck_);

            WW_ASSERT_OR( tzsp_len > 0 ) {
                spdlog::error( "invalid tzsp length." );
                return;
            }

            _PTR_MOVE( 4, 1 ); for(; ptr < end; ) {
                switch( *ptr ) {
                    case 0x00: _PTR_MOVE( 1, 1 ); continue;
                    case 0x01: _PTR_MOVE( 1, 1 ); break;
                    default: {
                        _PTR_MOVE( 1, 1 ); _PTR_MOVE( 2 + *ptr, 1 ); 
                    continue; }
                }
                break;
            }

            _PTR_MOVE( sizeof( ether_header ), sizeof( iphdr ) );
            ctch.type = catch_t::tzsp;
            resolve_packet( std::move( ctch ), ( const iphdr* )( ptr + sizeof( ether_header ) ) );
        break; }

        case ETHERTYPE_8021Q: {
            _PTR_MOVE( 4, 2 );

        goto l_dive_ether_type; }
    };
}

int main( int argc, char *argv[] ) {
    spdlog::set_default_logger( spdlog::default_logger()->clone( "wirewatch" ) );
    spdlog::set_pattern( A113_SPDLOG_PATTERN );

    char err[ PCAP_ERRBUF_SIZE ];
    
    pcap_if_t* ifs = nullptr;
    WW_ASSERT_OR( 0x0 == pcap_findalldevs( &ifs, err ) ) {
        spdlog::critical( "o devices found." ); return -0x1;
    }

    char* dev = ifs->name;
    spdlog::info( "watching over {}.", dev );

    static pcap_t* handle = pcap_open_live( dev, BUFSIZ, 1, 1000, err );
    WW_ASSERT_OR( handle ) {
        spdlog::critical( "ould not open {}.", dev ); return -0x1;
    }

    std::signal( SIGINT, [] ( int ) {
        pcap_breakloop( handle );
    } );
    (void)pcap_loop( handle, 0, packet_handler, NULL );

    pcap_freealldevs( ifs );
    pcap_close( handle );
    
    return 0x0;
}
