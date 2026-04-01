#include <bridge.hpp>
using namespace mdn;
using namespace a113;
using namespace a113::text;

struct _cli_dock_t : dock_t {
public:
    _cli_dock_t( void )
    : _cli{
        {},
        { 
        {   .text = "progctl",
            .opts = {
                { .sh0rt = 'c', .l0ng = "clear" },
                { .sh0rt = 'e', .l0ng = "exit" },
                { .sh0rt = 'v', .l0ng = "version" }
            },
            .fnc = [ this ] ( auto& stencil_ ) -> status_t {
                char opt; while( opt = stencil_.next() ) {
                    switch( opt ) { A113_TEXT_FASTCLI_DEFAULT_STENCIL_CASES
                        case 'c': {
                            
                        break; }
                        case 'e': {
                            BridgE.signal_stop();
                            return A113_OK;
                        }
                        case 'v': {
                            stencil_( "Madonna version: {}", MDN_VERSION_STR );
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
                    stencil_ += std::format( 
                        "{:+} {:+}i\n", root.real(), root.imag() 
                    );
                }
                return A113_OK;
            }
        }
        }
    } {}

public:
    struct _guix_t {
        std::string                      cmd_in   = {};
        a113::Dispenser< std::string >   cmd_out  = { a113::DispenserMode_Trylock };
    } _guix;

    a113::text::Fastcli   _cli;

public:
    MDN_DOCK_NAME_FNC override { return "cli"; }

    MDN_DOCK_GUIX_FNC override {
        const ImGuiWindowFlags_ window_flags = ImGuiWindowFlags_None;

        if( ImGui::Begin( "Command Line Interpreter", nullptr, window_flags ) ) {
            const auto cmd_in_flags = ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
            ImGui::SeparatorText( "Input" );
            const bool enter = ImGui::InputTextWithHint( std::format( " @ {}", MDN_VERSION_STR ).c_str(), "command...", &_guix.cmd_in, cmd_in_flags );
            ImGui::Separator();

            if( enter ) {
                std::string out;
                _cli.execute( _guix.cmd_in, &out );
                auto cmd_out = _guix.cmd_out.control();
                *cmd_out = std::move( out );
            }

            ImGui::BeginChild( "##_cli_out" );
                if( auto cmd_out = _guix.cmd_out.watch() ) {
                    ImGui::TextUnformatted( cmd_out->c_str() );
                }
            ImGui::EndChild();
        } 
        ImGui::End();
        return A113_OK; 
    }
};

MDN_AUTO_INSTALL( _cli_dock_t );

