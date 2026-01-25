#include <a113/osp/hyper_net.hpp>
#include <a113/OSp/IO_sockets.hpp>
#include <fstream>
#include <conio.h>

std::unique_ptr< a113::hyn::Executor > exec;
std::atomic_bool                       run;

#define GRAPHVIZ_PATH "hyn_graphviz.txt"
struct _flags_t {
    bool   make_graphviz   = false;
} flags;

void make_graphviz( void ) {
    if( not flags.make_graphviz ) return;
    
    std::ofstream{ GRAPHVIZ_PATH } << exec->Utils_make_graphviz();
}

int example_1( void ) {
    exec->push_port( new a113::hyn::Port{ { .str_id = "p0" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "p1" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "p2" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "p3" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "r0" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "r1" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "r2" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "r3" } } );

    exec->bind_PRP( "p0", "r0", "p1", { .flight_mode = 2 }, {} );
    exec->bind_RP( "r0", "p2", {} );
    exec->bind_RP( "r0", "p3", {} );

    exec->bind_PR( "p2", "r2", { .min_tok_cnt = 5 } );
    exec->bind_PR( "p3", "r3", { .min_tok_cnt = 5, .rte_tok_cnt = 3 } );

    exec->bind_PRP( "p1", "r1", "p0", {}, {} );

    exec->inject( "p0", new a113::hyn::Token{ { .str_id = "t0" } } );

    make_graphviz();

    spdlog::info( "Launching example 1..." );
for(;run;) {
    if( 'X' == _getch() ) break;
    auto crt_clk = exec->get_clock_counter();
    spdlog::info( "Executing clock {}...", crt_clk );
    exec->clock( 0.0 );
    spdlog::info( "Executed clock {}.", crt_clk );
    make_graphviz();
}
    return 0x0;
}

int example_2( void ) { 
    class Sem : public a113::hyn::Route, a113::io::IPv4_TCP_socket {
    public:
        Sem( a113::hyn::Route::config_t config_ )
        : Route{ config_ }
        {   
            spdlog::info( "Connecting to the controller..." );
            this->bind_peer( "127.0.0.1", 9009 );
            for( int attempt = 0x1; attempt <= 5; ++attempt ) {
                A113_ASSERT_OR( 0x0 == this->uplink() ) {
                    spdlog::warn( "Could not connect to the controller. Retrying..." );
                    continue;
                }
                goto l_conn_ok;
            }
            spdlog::error( "Failed to connect to the controller." );
            return;
        l_conn_ok:
            spdlog::info( "Connected to the controller." );
            std::thread{ &Sem::main, this }.detach();
        }

    public:
        std::atomic_bool is_green = { false };
    
    private:
        void main( void ) { for(;;) {
            char color;
            if( 0x1 == this->read( a113::io::port_R_desc_t{
                .dst_ptr = &color,
                .dst_n   = 1
            } ) ) {
                const bool is = color == 'G';
                spdlog::info( "Received {}.", is ? "green" : "red" );
                is_green.store( is );
            }
        } }
    };

    class Car : public a113::hyn::Token {
    public:
        Car( std::string route ) : _route{ std::move( route ) } {}

    private:
        std::string   _route;

    public:
        a113::status_t HyN_assert( a113::hyn::Route* rte_ ) {
            if( _runtime.prt->config.str_id == "s0" ) {
                Sem& sem = *dynamic_cast< Sem* >( rte_ );
                return sem.is_green.load() ? a113::hyn::AssertType_Pass : a113::hyn::AssertType_Fail;
            }

            if( _runtime.prt->config.str_id == "s1" ) {
                return rte_->config.str_id == _route ? a113::hyn::AssertType_Pass : a113::hyn::AssertType_Fail;
            }

            return a113::hyn::AssertType_Pass;
        }
    
    };

    exec->push_port( new a113::hyn::Port{ { .str_id = "s0" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "s1" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "sl" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "sf" } } );
    exec->push_port( new a113::hyn::Port{ { .str_id = "sr" } } );
    exec->push_route( new Sem{ { .str_id = "r0" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "rl" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "rf" } } );
    exec->push_route( new a113::hyn::Route{ { .str_id = "rr" } } );

    exec->bind_PRP( "s0", "r0", "s1", {}, {} );
    exec->bind_PRP( "s1", "rl", "sl", {}, {} );
    exec->bind_PRP( "s1", "rf", "sf", {}, {} );
    exec->bind_PRP( "s1", "rr", "sr", {}, {} );

    make_graphviz();

    spdlog::info( "Launching example 2..." );
for(;run;) {
    auto c = _getch(); if( 'X' == c ) break;

    if( 'C' == c ) {
        switch( _getch() ) {
            case 'L': exec->inject( "s0", a113::HVec< Car >::make( "rl" ) ); break;
            case 'F': exec->inject( "s0", a113::HVec< Car >::make( "rf" ) ); break;
            case 'R': exec->inject( "s0", a113::HVec< Car >::make( "rr" ) ); break;
        }
    }

    auto crt_clk = exec->get_clock_counter();
    spdlog::info( "Executing clock {}...", crt_clk );
    exec->clock( 0.0 );
    spdlog::info( "Executed clock {}.", crt_clk );
    make_graphviz();
}
    return 0x0;
}

int ( *examples[] )( void ) = {
    &example_1,
    &example_2
};
constexpr int example_count = sizeof( examples ) / sizeof( void* );

int main( int argc, char* argv[] ) {
    a113::init( argc, argv, a113::init_args_t{
        .flags = a113::InitFlags_Sockets
    } );

    A113_ASSERT_OR( argc > 1 ) {
        A113_LOGE( "Please call this program with the following arguments: <example_no> [--make-graphviz]. Currently {} example(s) are available.", example_count );
        return -0x1;
    }

    for( auto idx{ 0x1uz }; idx < argc; ++idx ) {
        if( NULL == strcmp( "--make-graphviz", argv[ idx ] ) ) flags.make_graphviz = true;
    }

    a113::set_log_level( a113::LogComponent_SCT, spdlog::level::debug );

    try {
        int example_no = std::atoi( argv[ 1 ] );
        A113_ASSERT_OR( example_no >= 0x1 && example_no <= example_count ) throw std::runtime_error{ "Example number out of bounds." };

        exec = std::make_unique< a113::hyn::Executor >( std::format( "Example-{}", example_no ).c_str() );
        //exec->set_log_level( spdlog::level::debug );

        run = true;
        return examples[ example_no - 1 ]();
    } catch( std::exception& ex ) {
        spdlog::critical( "{}", ex.what() );
        return -0x1;
    }

    return 0x0;
}