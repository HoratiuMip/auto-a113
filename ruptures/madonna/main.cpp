#include <bridge.hpp>

int main( int argc, char* argv[] ) {
    a113::init( argc, argv, a113::init_args_t{
        .flags = a113::InitFlags_None
    } );

    mdn::BridgE.start();
    mdn::BridgE.wait_stop();

    return 0x0;
}