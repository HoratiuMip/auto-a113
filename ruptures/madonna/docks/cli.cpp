#include <bridge.hpp>
using namespace mdn;

struct _cli_t : dock_t {
public:
    struct _guix_t {
        std::string   cmd_in   = {};
    } _guix;

public:
    MDN_DOCK_NAME_FNC override { return "cli"; }

    MDN_DOCK_GUIX_FNC override {
        const ImGuiWindowFlags_ window_flags = ImGuiWindowFlags_None;

        if( ImGui::Begin( this->_id.c_str(), nullptr, window_flags ) ) {
            const auto cmd_in_flags = ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
            ImGui::SeparatorText( "Input" );
            const bool enter = ImGui::InputTextWithHint( std::format( " @ {}", MDN_VERSION_STR ).c_str(), "command...", &_guix.cmd_in, cmd_in_flags );
            ImGui::Separator();
        } 
        ImGui::End();
        return A113_OK; 
    }

};

MDN_DOCK_FAST_AUTO_INSTALL( _cli_t );

