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
                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'v': {
                            spdlog::info( "vasd" );
                        break; }
                    }
                }
                return A113_OK;
            }
        }, {
            .text = "roots",
            .opts = {
                { .sh0rt = 'c', .l0ng = "coeffs", .arg = Fastcli::Arg_f64, .fast_id = 0x0, .argc = Fastcli::Argc_multi_compact }
            },
            .fnc = [] ( auto& stencil_ ) -> status_t {
                std::vector< double > coeffs;

                char opt;  while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            coeffs = std::move( stencil_.arg_f64v() );
                        break; }
                    }
                }
        
                for( auto& root : mdn_1::roots( coeffs ) ) {
                    std::cout << root << '\n';
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