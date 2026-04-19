#include <a113/brp/IO_string_utils.hpp>
#include <a113/osp/core.hpp>
#include <pcap.h>

#define WW_ASSERT_OR(c) A113_ASSERT_OR(c)

struct catch_t {
    time_t   timestamp   = 0x0;

    struct _src_t : a113::io::ipv4_addr_pack_t { 
        a113::io::ipv4_addr_pack_t   addr;     
    } src;

    struct _dst_t : a113::io::ipv4_addr_pack_t {
        a113::io::ipv4_addr_pack_t   addr;  
    } dst;
};

std::map< std::string, catch_t >   map;

void resolve_catch( catch_t ctch_ ) {
    map[ std::format( "{}->{}", ctch_.src.addr.c_str(), ctch_.dst.addr.c_str() ) ] = std::move( ctch_ );

    std::system( "clear" );
    auto t_now = time( nullptr );
    for( auto itr = map.begin(); itr != map.end(); ) {
        auto& [ key, ctch ] = *itr;

        if( t_now - ctch.timestamp < 5 ) {
            spdlog::info( "{}\t\t| {}", key, ctch.timestamp );
            ++itr;
        } else {
            itr = map.erase( itr );
        }
    }
}

void packet_handler( u_char* args_, const pcap_pkthdr* hdr_, const u_char* pck_ ) {
    catch_t ctch    = {};
    auto*   eth_hdr = ( ether_header* )pck_;

    ctch.timestamp = hdr_->ts.tv_sec;

    if( ntohs( eth_hdr->ether_type ) == ETHERTYPE_IP ) {
        iphdr* ip_hdr = ( iphdr* )( pck_ + sizeof( ether_header ) );

        ctch.src.addr = a113::io::ipv4_addr_pack_t{ ip_hdr->saddr };
        ctch.dst.addr = a113::io::ipv4_addr_pack_t{ ip_hdr->daddr };

        resolve_catch( std::move( ctch ) );
    }
}

int main( int argc, char *argv[] ) {
    spdlog::set_pattern( A113_SPDLOG_PATTERN );

    char err[ PCAP_ERRBUF_SIZE ];
    
    pcap_if_t* ifs = nullptr;
    WW_ASSERT_OR( 0x0 == pcap_findalldevs( &ifs, err ) ) {
        spdlog::critical( "No devices found." ); return -0x1;
    }

    char* dev = ifs->name;
    spdlog::info( "Watching over {}.", dev );

    static pcap_t* handle = pcap_open_live( dev, BUFSIZ, 1, 1000, err );
    WW_ASSERT_OR( handle ) {
        spdlog::critical( "Could not open {}.", dev ); return -0x1;
    }

    std::signal( SIGINT, [] ( int ) {
        pcap_breakloop( handle );
    } );
    (void)pcap_loop( handle, 0, packet_handler, NULL );

    pcap_freealldevs( ifs );
    pcap_close( handle );
    
    return 0x0;
}
