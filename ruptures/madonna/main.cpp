#include <bridge.hpp>
#include <csignal>

int main( int argc, char* argv[] ) {
    a113::init( argc, argv, a113::init_args_t{
        .flags = a113::InitFlags_None
    } );

    std::signal( SIGINT, [] ( int sig_ ) static -> void {
        mdn::BridgE.signal_stop();
    } );

    mdn::BridgE.start();
    mdn::BridgE.wait_stop();

    a113::mdn_1::vec_t v;

    return 0x0;
}