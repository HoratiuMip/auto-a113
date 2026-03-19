#include <iostream>
#include <a113/osp/madonna.hpp>

using namespace a113;

int main( int argc, char* argv[] ) {
    // a113::text::Fastcli fastcli{
    //     {}, {
    //         { "test",
    //             {

    //             },
    //             [] ( auto& opts_lens ) -> auto {
                    
    //                 return A113_OK;
    //             }
    //         }
    //     }
    // };

    // std::string resp;
    // fastcli.parse( "test", &resp );
    // spdlog::error( "{}", resp );

    for( auto& root : mdn_1::roots< float >( { -4, 2, 4, 10, 5, -3, -13, 27, 12, -24 } ) )
        std::cout << root << '\n';

    return 0x0;
}