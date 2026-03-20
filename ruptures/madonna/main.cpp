#include <a113/osp/madonna.hpp>
#include <a113/osp/fastcli.hpp>

#include <iostream>

using namespace std; using namespace a113;

int main( int argc, char* argv[] ) {
    using namespace a113::text;
    Fastcli fastcli{
        {},
        { 
        {   .text = "proginfo",
            .opts = {
                { .sh0rt = 'v', .l0ng = "version" }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                while( true ) {
                    auto [ opt, arg ] = stencil_.next();
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES

                        case 'v': {
                            spdlog::info( "vasd" );
                        break; }
                    }
                }
                return A113_OK;
            }
        }, {
            .text = "echo",
            .opts = {
                { .sh0rt = 'e', .arg = Fastcli::opt_t::Arg_text, .fast_id = 0x0 }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                while( true ) {
                    auto [ opt, arg ] = stencil_.next();
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES

                        case 'e': {
                            spdlog::info( "{}", *(string*)arg );
                        break; }
                    }
                }
                return A113_OK;
            }
        }
        }
    };

    for(;;) {
        std::string cmd, out;
        std::cout << "\n>> ";
        std::getline( std::cin, cmd );
        fastcli.execute( cmd, &out );
        std::cout << out;
    }

}