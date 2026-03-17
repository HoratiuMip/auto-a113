#include <iostream>
#include <a113/osp/fastcli.hpp>

int main( int argc, char* argv[] ) {
    a113::text::Fastcli fastcli{
        {}, {
            { "test",
                {

                },
                [] ( auto& opts_lens ) -> auto {
                    
                    return A113_OK;
                }
            }
        }
    };

    std::string resp;
    fastcli.parse( "test", &resp );
    spdlog::error( "{}", resp );

    return 0x0;
}