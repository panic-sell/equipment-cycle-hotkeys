#pragma once

#include "ui_drawing.h"
#include "ui_viewmodels.h"

namespace ech {
namespace ui {

class MenuBar final {
  public:
    void
    DrawImportMenu() {}

  private:
    std::string export_fn_;
};

inline void
DrawMenuBar(
    std::span<const std::string> filenames,
    std::function<void(std::string_view filename)> import_callback,
    std::function<void(std::string_view filename)> export_callback
) {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("Profiles")) {
        ImGui::Text("hi");
        ImGui::Separator();
        static std::string buf;
        if (ImGui::InputTextWithHint(
                "##profile_name", "Name...", &buf, ImGuiInputTextFlags_EnterReturnsTrue
            )) {
        }
        ImGui::SameLine();
        ImGui::Button("Create");

        ImGui::EndMenu();
    }

    constexpr const char* import_popup_id = "import";
    if (ImGui::Button("Import")) {
        ImGui::OpenPopup(import_popup_id);
    }
    if (ImGui::BeginPopup(import_popup_id)) {
        if (filenames.empty()) {
            ImGui::Text("No files to import.");
        } else {
            for (const auto& fn : filenames) {
                if (ImGui::MenuItem(fn.c_str())) {
                    import_callback(fn);
                }
            }
        }

        ImGui::EndPopup();
    }
    ImGui::SameLine(0);

    constexpr const char* export_popup_id = "export";
    if (ImGui::Button("Export")) {
        ImGui::OpenPopup(export_popup_id);
    }
    if (ImGui::BeginPopup(export_popup_id)) {
        for (const auto& fn : filenames) {
            if (ImGui::MenuItem(fn.c_str())) {
                export_callback(fn);
            }
        }
        ImGui::SeparatorText("New File");
        static std::string buf;
        if (ImGui::InputTextWithHint(
                "##new_file", "Filename...", &buf, ImGuiInputTextFlags_EnterReturnsTrue
            )) {
            export_callback(buf);
        }

        ImGui::EndPopup();
    }
    ImGui::SameLine(0);

    constexpr const char* settings_popup_id = "settings";
    if (ImGui::Button("Settings")) {
        ImGui::OpenPopup(settings_popup_id);
    }
    if (ImGui::BeginPopup(settings_popup_id)) {
        ImGui::SeparatorText("Font Scale");
        ImGui::DragFloat(
            "##font_scale",
            &ImGui::GetIO().FontGlobalScale,
            /*v_speed=*/.01f,
            /*v_min=*/.5f,
            /*v_max=*/2.f,
            /*format=*/"%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );

        static int color = 0;
        ImGui::SeparatorText("Colors");
        if (ImGui::MenuItem("Dark", nullptr, color == 0)) {
            ImGui::StyleColorsDark();
            color = 0;
        }
        if (ImGui::MenuItem("Light", nullptr, color == 1)) {
            ImGui::StyleColorsLight();
            color = 1;
        }
        if (ImGui::MenuItem("Classic", nullptr, color == 2)) {
            ImGui::StyleColorsClassic();
            color = 2;
        }

        ImGui::EndPopup();
    }

    ImGui::EndMenuBar();
}

}  // namespace ui
}  // namespace ech
