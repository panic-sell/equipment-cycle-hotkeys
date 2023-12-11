#pragma once

#include "keys.h"
#include "ui.h"
#include "ui_viewmodels.h"

namespace ech {
namespace ui {

using TableCallback = std::function<void()>;

template <typename T>
struct Table final {
    const char* id;
    std::span<const char* const> headers;
    std::span<const T> row_entities;
    std::function<TableCallback(const T& entity, int row, int col)> draw_cell;
    std::function<TableCallback(int row)> draw_ctrl_cell;
    // Optional fields.
    ImVec2 cell_padding = {2, 4};
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerH;

    int
    rows() const {
        return row_entities.size();
    }

    int
    cols() const {
        return headers.size() + 1;
    }

    int
    ctrl_col() const {
        return headers.size();
    }

    int
    CellIndex(int r, int c) const {
        return r * cols() + c;
    }

    TableCallback
    Draw() const {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
        if (!ImGui::BeginTable(id, cols, table_flags)) {
            ImGui::PopStyleVar();
            return {};
        }

        // Headers.
        for (int c = 0; c < ctrl_col(); c++) {
            ImGui::TableSetupColumn(headers[c]);
        }
        ImGui::TableSetupColumn("##controls", ImGuiTableColumnFlags_WidthFixed);
        if (std::any_of(headers.cbegin(), headers.cend(), [](const char* s) { return *s; })) {
            ImGui::TableHeadersRow();
        }

        TableCallback cb;
        for (int r = 0; r < rows(); r++) {
            const auto& ent = row_entities[r];
            ImGui::TableNextRow();

            // Main cells.
            for (int c = 0; c < ctrl_col(); c++) {
                ImGui::TableSetColumnIndex(c);
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::PushID(CellIndex(r, c));
                cb = draw_cell(ent, r, c);
                ImGui::PopID();
            }

            // Control button cell.
            ImGui::TableSetColumnIndex(ctrl_col());
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);
            ImGui::PushID(CellIndex(r, ctrl_col()));
            cb = draw_ctrl_cell(r);
            ImGui::PopID();
            ImGui::PopStyleColor();
        }

        ImGui::EndTable();
        ImGui::PopStyleVar();

        return cb;
    }
};

inline void
DrawCtrlCell(int r) {
    if (ImGui::ArrowButton("up", ImGuiDir_Up) && row > 0) {
        drag_source = row;
        drag_target = row - 1;
    }
    SetupDragSource(table_id, row, draw_drag);
    if (auto o = SetupDragTarget(table_id, row)) {
        drag_source = o->first;
        drag_target = o->second;
    }
    ImGui::SameLine(0, 0);

    if (ImGui::ArrowButton("down", ImGuiDir_Down) && row < rows - 1) {
        drag_source = row;
        drag_target = row + 1;
    }
    SetupDragSource(table_id, row, draw_drag);
    if (auto o = SetupDragTarget(table_id, row)) {
        drag_source = o->first;
        drag_target = o->second;
    }
    ImGui::SameLine(0, 0);

    if (ImGui::Button("X")) {
        remove = row;
    }
}

inline Table<Keyset>
DrawKeysets(std::vector<Keyset>& keysets) {
    constexpr auto headers = std::array{"", "", "", ""};

    auto draw_combo = [](const Keyset& keyset, int, int col) -> TableCallback {
        const auto& keycode = keyset[col];
        const char* preview = internal::kKeycodeNamesForUI[KeycodeNormalize(keycode)];
        constexpr auto combo_flags = ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;
        if (!ImGui::BeginCombo("##combo", preview, combo_flags)) {
            return {};
        }

        TableCallback cb;
        for (uint32_t opt_keycode = 0; opt_keycode < internal::kKeycodeNamesForUI.size();
             opt_keycode++) {
            const char* opt = internal::kKeycodeNamesForUI[opt_keycode];
            if (!*opt) {
                continue;
            }
            auto is_selected = opt_keycode == keycode;
            if (ImGui::Selectable(opt, is_selected)) {
                cb = [&keycode, opt_keycode]() { const_cast<uint32_t&>(keycode) = opt_keycode; };
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
        return cb;
    };
}

}  // namespace ui
}  // namespace ech
