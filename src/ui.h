#pragma once

#include "ui_viewmodels.h"

namespace ech {
namespace ui {
namespace internal {

constexpr inline auto kDropdownFlags = ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;

constexpr inline auto kKeycodeNamesForUI = []() {
    auto arr = kKeycodeNames;
    arr[0] = "(Unbound)";
    return arr;
}();

/// Contains a placeholder for `EsvmItem::Choice::kGear`.
///
/// Let `i = static_cast<size_t>(c)`. `kEsvmItemDropdownTemplate[i]` is the option corresponding to
/// choice `c`.
constexpr inline auto kEsvmItemDropdownTemplate = []() {
    auto arr = std::array{"", "", ""};
    arr[static_cast<size_t>(EsvmItem::Choice::kIgnore)] = "(Ignore)";
    arr[static_cast<size_t>(EsvmItem::Choice::kUnequip)] = "(Unequip)";
    return arr;
}();

inline const char*
EsvmItemToCStr(const EsvmItem& item) {
    if (item.canonical_choice() == EsvmItem::Choice::kGear) {
        return item.gos.gear()->form().GetName();
    }
    return kEsvmItemDropdownTemplate[static_cast<size_t>(item.canonical_choice())];
}

inline void
SetupDragSource(const char* payload_id, int row, std::function<void()> draw_drag_tooltip) {
    if (!ImGui::BeginDragDropSource()) {
        return;
    }
    ImGui::SetDragDropPayload(payload_id, &row, sizeof(row));
    draw_drag_tooltip();
    ImGui::EndDragDropSource();
};

/// If drag target is triggered, returns `(drag source row, drag target row)`.
inline std::optional<std::pair<int, int>>
SetupDragTarget(const char* payload_id, int row) {
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    std::optional<std::pair<int, int>> out;
    if (auto* payload = ImGui::AcceptDragDropPayload(payload_id)) {
        out.emplace(*static_cast<const decltype(row)*>(payload->Data), row);
    }
    ImGui::EndDragDropTarget();
    return out;
}

/// Returned by `DrawTable()` to indicate row changes.
struct TableRowMutations {
    /// If less than 0, no removals occurred.
    int removed = -1;
    /// If less than 0, no drag action occurred.
    int dragged_from = -1;
    /// If less than 0, no drag action occurred.
    int dragged_to = -1;
};

template <typename T>
inline TableRowMutations
DrawTable(
    const char* table_id,
    std::vector<T>& row_entities,

    /// Assume the number of table columns (excluding the control column) is equal to
    /// `headers.size()`. Elements must not be nullptr. If all elements are the empty string, then
    /// the header will not be shown.
    std::span<const char* const> headers,

    /// Draws UI elements for the given cell.
    ///
    /// `row` is `row_entity`'s index within `row_entities`.
    ///
    /// It's guaranteed that `0 <= col < headers.size()` (i.e. this function will never be called on
    /// a control button cell).
    std::function<void(T& row_entity, int row, int col)> draw_cell,

    std::function<void(const T& row_entity)> draw_drag_tooltip
) {
    // Index of the column containing control buttons.
    const auto last_col = static_cast<int>(headers.size());
    const auto columns = last_col + 1;
    const auto rows = row_entities.size();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 4));
    if (!ImGui::BeginTable(
            table_id, columns, ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerH
        )) {
        ImGui::PopStyleVar();
        return {};
    }

    // Headers.
    for (int col = 0; col < last_col; col++) {
        ImGui::TableSetupColumn(headers[col]);
    }
    ImGui::TableSetupColumn("##controls", ImGuiTableColumnFlags_WidthFixed);
    if (std::any_of(headers.cbegin(), headers.cend(), [](const char* s) { return *s; })) {
        ImGui::TableHeadersRow();
    }

    auto remove = -1;
    auto drag_source = -1;
    auto drag_target = -1;

    for (int row = 0; row < rows; row++) {
        auto& row_entity = row_entities[row];
        ImGui::TableNextRow();

        // Main cells.
        for (int col = 0; col < last_col; col++) {
            ImGui::TableSetColumnIndex(col);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::PushID(row * columns + col);
            draw_cell(row_entity, row, col);
            ImGui::PopID();
        }

        // Control buttons.
        {
            auto draw_drag = [&]() { draw_drag_tooltip(row_entity); };
            ImGui::TableSetColumnIndex(columns - 1);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);
            ImGui::PushID(row * columns + columns - 1);

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

            ImGui::PopID();
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndTable();
    ImGui::PopStyleVar();

    if (remove >= 0 && remove < rows) {
        row_entities.erase(row_entities.begin() + remove);
        return {.removed = remove};
    }
    if (  // clang-format off
        drag_source != drag_target
        && drag_source >= 0
        && drag_source < rows
        && drag_target >= 0
        && drag_target < rows
    ) {  // clang-format on
        auto item = std::move(row_entities[drag_source]);
        row_entities.erase(row_entities.begin() + drag_source);
        row_entities.insert(row_entities.begin() + drag_target, std::move(item));
        return {
            .dragged_from = drag_source,
            .dragged_to = drag_target,
        };
    }
    return {};
}

inline void
DrawHotkeyList(HotkeysVM<>& hotkeys_vm, int& selected) {
    constexpr auto headers = std::array{""};

    auto draw_item = [&](HotkeyVM<>& hotkey_vm, int row, int) {
        auto is_selected = row == selected;
        const char* label = hotkey_vm.name.empty() ? "(Unnamed)" : hotkey_vm.name.c_str();
        if (ImGui::RadioButton(label, is_selected)) {
            selected = row;
        }
    };

    auto draw_drag_tooltip = [&](const HotkeyVM<>& hotkey_vm) {
        ImGui::Text("%s", hotkey_vm.name.c_str());
    };

    auto row_changes = DrawTable<HotkeyVM<>>(
        "hotkeys_list", hotkeys_vm, headers, draw_item, draw_drag_tooltip
    );
    if (row_changes.removed >= 0) {
        selected = std::min(selected, static_cast<int>(hotkeys_vm.size()) - 1);
    } else if (row_changes.dragged_to >= 0 && row_changes.dragged_to < hotkeys_vm.size()) {
        selected = row_changes.dragged_to;
    }

    if (ImGui::Button("New", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
        hotkeys_vm.emplace_back();
        selected = static_cast<int>(hotkeys_vm.size()) - 1;
    }
    if (hotkeys_vm.empty()) {
        return;
    }

    if (selected < 0 || selected >= hotkeys_vm.size()) {
        selected = 0;
    }
    // At this point, it's guaranteed that `selected` is in bounds.
    if (ImGui::Button("Duplicate selected", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
        hotkeys_vm.insert(hotkeys_vm.begin() + selected + 1, hotkeys_vm[selected]);
        selected++;
    }
}

inline void
DrawKeysets(std::vector<Keyset>& keysets) {
    constexpr auto headers = std::array{"", "", "", ""};

    auto draw_dropdown = [](Keyset& keyset, int, int col) {
        auto& keycode = keyset[col];
        const char* preview = kKeycodeNamesForUI[KeycodeNormalize(keycode)];
        if (!ImGui::BeginCombo("##dropdown", preview, kDropdownFlags)) {
            return;
        }
        for (uint32_t opt_keycode = 0; opt_keycode < kKeycodeNamesForUI.size(); opt_keycode++) {
            const char* opt = kKeycodeNamesForUI[opt_keycode];
            if (!*opt) {
                continue;
            }
            auto is_selected = opt_keycode == keycode;
            if (ImGui::Selectable(opt, is_selected)) {
                keycode = opt_keycode;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    };

    auto draw_drag_tooltip = [&](const Keyset& keyset) {
        auto names = std::array{
            kKeycodeNamesForUI[KeycodeNormalize(keyset[0])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[1])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[2])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[3])],
        };
        ImGui::Text("%s+%s+%s+%s", names[0], names[1], names[2], names[3]);
    };

    ImGui::SeparatorText("Keysets");
    DrawTable<Keyset>("keyset_table", keysets, headers, draw_dropdown, draw_drag_tooltip);
    if (ImGui::Button("New", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
        keysets.emplace_back();
    }
}

inline void
DrawEquipsets(std::vector<EquipsetVM>& equipsets_vm) {
    auto headers = equipsets_vm.empty() ? std::array{"", "", "", ""}
                                        : std::array{"Left", "Right", "Ammo", "Voice"};

    auto draw_dropdown = [](EquipsetVM& equipset_vm, int, int col) {
        auto& esvm_item = equipset_vm[col];
        if (!ImGui::BeginCombo("##dropdown", EsvmItemToCStr(esvm_item), kDropdownFlags)) {
            return;
        }

        auto opts = kEsvmItemDropdownTemplate;
        if (auto* gear = esvm_item.gos.gear()) {
            opts[static_cast<size_t>(EsvmItem::Choice::kGear)] = gear->form().GetName();
        }
        for (size_t i = 0; i < opts.size(); i++) {
            const char* opt = opts[i];
            if (!*opt) {
                continue;
            }
            auto opt_choice = static_cast<EsvmItem::Choice>(i);
            auto is_selected = opt_choice == esvm_item.canonical_choice();
            if (ImGui::Selectable(opt, is_selected)) {
                esvm_item.choice = opt_choice;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    };

    auto draw_drag_tooltip = [](const EquipsetVM& equipset_vm) {
        ImGui::Text(
            "%s, %s, %s, %s",
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kLeft)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kRight)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kAmmo)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kShout)])
        );
    };

    ImGui::SeparatorText("Equipsets");
    DrawTable<EquipsetVM>(
        "equipset_table", equipsets_vm, headers, draw_dropdown, draw_drag_tooltip
    );
    if (ImGui::Button("Add currently equipped", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
        equipsets_vm.emplace_back();
    }
}

}  // namespace internal

inline void
Draw(HotkeysVM<>& hotkeys_vm, int& selected) {
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (!main_viewport) {
        return;
    }
    auto mv_size = main_viewport->WorkSize;
    ImGui::SetNextWindowPos(mv_size * ImVec2(.1f, .1f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(mv_size * ImVec2(.4f, .8f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(mv_size * ImVec2(.4f, .4f), ImVec2(FLT_MAX, FLT_MAX));
    auto open = false;
    ImGui::Begin(
        "Equipment Cycle Hotkeys",
        &open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
    );

    ImGui::SetNextWindowSizeConstraints(mv_size * ImVec2(.15f, 0), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::BeginChild(
        "hotkey_list",
        mv_size * ImVec2(.15f, 0),
        ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX,
        ImGuiWindowFlags_NoSavedSettings
    );
    ech::ui::internal::DrawHotkeyList(hotkeys_vm, selected);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("selected_hotkey", ImVec2(0, 0));
    if (selected >= 0 && selected < hotkeys_vm.size()) {
        auto& hotkey_vm = hotkeys_vm[selected];
        ImGui::InputTextWithHint("Hotkey name", "(Unnamed)", &hotkey_vm.name);
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        ech::ui::internal::DrawKeysets(hotkey_vm.keysets);
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        ech::ui::internal::DrawEquipsets(hotkey_vm.equipsets);
    }
    ImGui::EndChild();

    ImGui::End();
}

}  // namespace ui
}  // namespace ech
