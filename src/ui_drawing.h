#pragma once

#include "keys.h"
#include "ui_state.h"

namespace ech {
namespace ui {
namespace internal {

using Action = std::function<void()>;

struct TableRowChanges final {
    size_t remove = size_t(-1);
    size_t drag_source = size_t(-1);
    size_t drag_target = size_t(-1);
};

/// A table where rows can be reordered and deleted. Control buttons are located in the rightmost
/// column.
///
/// N is the number of columns excluding the control button column.
template <typename T, size_t N>
struct Table final {
  public:
    /// ImGui ID for the table element. Must not be nullptr or empty.
    const char* id;

    /// Elements must not be nullptr. If all elements are the empty string, then the header will not
    /// be shown.
    std::array<const char*, N> headers;

    /// The underlying objects should be mutable, as `Action` objects returned from draw functions
    /// are expected to modify them.
    std::vector<T>& viewmodel;

    /// Returns an action that should be performed when a change is made to the cell. For example,
    /// if this function draws a combo box, it should return a callback for selecting an item.
    std::function<Action(const T& obj, size_t row, size_t col)> draw_cell;

    /// Typically just a wrapper for `ImGui::Text`.
    std::function<void(const T& obj)> draw_drag_tooltip;

    /// Returns:
    /// 1. A callback to update `viewmodel` (the callback may be empty).
    /// 1. The row changes that that callback will actuate.
    ///
    /// Since `viewmodel` is the only object captured-by-reference by the returned callback, it's
    /// fine for the callback to outlive this table object.
    std::pair<Action, TableRowChanges>
    Draw() const {
        constexpr auto cell_padding = ImVec2(2, 4);
        constexpr auto table_flags = ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerH;

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);
        if (!ImGui::BeginTable(id, static_cast<int>(cols()), table_flags)) {
            ImGui::PopStyleVar();
            return {};
        }

        // Headers.
        for (int size_t = 0; size_t < ctrl_col(); size_t++) {
            ImGui::TableSetupColumn(headers[size_t]);
        }
        ImGui::TableSetupColumn("##controls", ImGuiTableColumnFlags_WidthFixed);
        if (std::any_of(headers.cbegin(), headers.cend(), [](const char* s) { return *s; })) {
            ImGui::TableHeadersRow();
        }

        std::pair<Action, TableRowChanges> draw_res;
        for (size_t r = 0; r < rows(); r++) {
            const auto& obj = viewmodel[r];
            ImGui::TableNextRow();

            // Main row cells.
            for (size_t c = 0; c < ctrl_col(); c++) {
                ImGui::TableSetColumnIndex(static_cast<int>(c));
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::PushID(cell_id(r, c));
                if (auto a = draw_cell(obj, r, c)) {
                    draw_res = {a, {}};
                }
                ImGui::PopID();
            }

            // Control buttons.
            ImGui::TableSetColumnIndex(static_cast<int>(ctrl_col()));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);
            ImGui::PushID(cell_id(r, ctrl_col()));
            if (auto atrc = DrawDragButton(r, ImGuiDir_Up); atrc.first) {
                draw_res = atrc;
            }
            ImGui::SameLine(0, 0);
            if (auto atrc = DrawDragButton(r, ImGuiDir_Down); atrc.first) {
                draw_res = atrc;
            }
            ImGui::SameLine(0, 0);
            if (auto atrc = DrawCloseButton(r); atrc.first) {
                draw_res = atrc;
            }
            ImGui::PopID();
            ImGui::PopStyleColor();
        }

        ImGui::EndTable();
        ImGui::PopStyleVar();
        return draw_res;
    }

  private:
    size_t
    rows() const {
        return viewmodel.size();
    }

    size_t
    cols() const {
        return headers.size() + 1;
    }

    size_t
    ctrl_col() const {
        return headers.size();
    }

    int
    cell_id(size_t r, size_t c) const {
        return static_cast<int>(r * cols() + c);
    }

    std::pair<Action, TableRowChanges>
    DrawCloseButton(size_t row) const {
        if (!ImGui::Button("X")) {
            return {{}, {}};
        }
        auto a = [&viewmodel = viewmodel, row]() { viewmodel.erase(viewmodel.begin() + row); };
        auto trc = TableRowChanges{.remove = row};
        return {a, trc};
    }

    std::pair<Action, TableRowChanges>
    DrawDragButton(size_t row, ImGuiDir dir) const {
        Action a;
        TableRowChanges trc;
        if (dir == ImGuiDir_Up) {
            if (ImGui::ArrowButton("up", dir) && row > 0) {
                a = [&viewmodel = viewmodel, row]() {
                    std::swap(viewmodel[row], viewmodel[row - 1]);
                };
                trc = {.drag_source = row, .drag_target = row - 1};
            }
        } else if (dir == ImGuiDir_Down) {
            if (ImGui::ArrowButton("down", dir) && row + 1 < rows()) {
                a = [&viewmodel = viewmodel, row]() {
                    std::swap(viewmodel[row], viewmodel[row + 1]);
                };
                trc = {.drag_source = row, .drag_target = row + 1};
            };
        } else {
            return {{}, {}};
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(id, &row, sizeof(row));
            draw_drag_tooltip(viewmodel[row]);
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const auto* payload = ImGui::AcceptDragDropPayload(id)) {
                auto src_row = *static_cast<const size_t*>(payload->Data);
                a = [&viewmodel = viewmodel, src_row, row]() {
                    auto item = std::move(viewmodel[src_row]);
                    viewmodel.erase(viewmodel.begin() + src_row);
                    viewmodel.insert(viewmodel.begin() + row, std::move(item));
                };
                trc = TableRowChanges{.drag_source = src_row, .drag_target = row};
            }
            ImGui::EndDragDropTarget();
        }

        return {a, trc};
    }
};

inline Action
DrawImportMenu(UI& ui) {
    if (!ImGui::BeginMenu("Import")) {
        return {};
    }

    Action action;
    if (ui.profile_cache.empty()) {
        ImGui::Text("No saved profiles.");
    } else {
        for (const auto& profile : ui.profile_cache) {
            if (!ImGui::MenuItem(profile.c_str())) {
                continue;
            }
            action = [&ui, profile]() { ui.ImportProfile(profile); };
        }
    }

    ImGui::EndMenu();
    return action;
}

inline Action
DrawExportMenu(UI& ui) {
    Action action;
    auto confirm_export = false;
    if (ImGui::BeginMenu("Export")) {
        ImGui::InputTextWithHint("##new_profile", "Enter profile name...", &ui.export_profile);
        if (ImGui::IsItemDeactivated()) {
            action = [&ui]() { ui.NormalizeExportProfile(); };
        }
        if (ImGui::Button("Export Profile")) {
            confirm_export = !ui.export_profile.empty();
        }
        ImGui::EndMenu();
    }

    if (confirm_export) {
        ImGui::OpenPopup("confirm_export");
    }
    if (ImGui::BeginPopup("confirm_export")) {
        if (const auto* cached_profile = ui.FindCachedProfileMatchingExportProfile()) {
            ImGui::Text("Overwrite profile '%s'?", cached_profile->c_str());
        } else {
            ImGui::Text("Save as profile '%s'?", ui.export_profile.c_str());
        }
        if (ImGui::Button("Yes")) {
            action = [&ui]() { ui.ExportProfile(); };
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return action;
}

inline Action
DrawHotkeyList(UI& ui) {
    auto table = Table<HotkeyUI<EquipsetUI>, 1>{
        .id = "hotkeys_list",
        .headers = std::array{""},
        .viewmodel = ui.hotkeys_ui,
        .draw_cell = [&ui](const HotkeyUI<EquipsetUI>& hotkey, size_t row, size_t) -> Action {
            const char* label = hotkey.name.empty() ? "(Unnamed)" : hotkey.name.c_str();
            if (!ImGui::RadioButton(label, row == ui.selected_hotkey)) {
                return {};
            }
            return [&ui, row]() { ui.selected_hotkey = row; };
        },
        .draw_drag_tooltip = [](const HotkeyUI<EquipsetUI>& hotkey
                             ) { ImGui::Text("%s", hotkey.name.c_str()); },
    };

    auto atrc = table.Draw();
    if (ImGui::Button("New", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        auto a = [&ui]() {
            ui.hotkeys_ui.emplace_back();
            // Adding a new hotkey selects that hotkey.
            ui.selected_hotkey = ui.hotkeys_ui.size() - 1;
        };
        atrc = {std::move(a), {}};
    }
    if (!atrc.first) {
        return {};
    }

    return [atrc, &ui]() {
        const auto& [a, trc] = atrc;
        if (trc.remove < ui.hotkeys_ui.size()) {
            // If the selected hotkey is below the removed hotkey, then move selection upward.
            if (trc.remove < ui.selected_hotkey) {
                ui.selected_hotkey--;
            }
        } else if (trc.drag_source < ui.hotkeys_ui.size() && trc.drag_target < ui.hotkeys_ui.size()) {
            // Select the row that was dragged.
            ui.selected_hotkey = trc.drag_target;
        }
        a();
        if (ui.selected_hotkey >= ui.hotkeys_ui.size() && ui.selected_hotkey > 0) {
            ui.selected_hotkey--;
        }
    };
}

inline void
DrawName(HotkeyUI<EquipsetUI>& hotkey) {
    ImGui::InputTextWithHint("Hotkey Name", "Enter hotkey name...", &hotkey.name);
}

inline Action
DrawKeysets(std::vector<Keyset>& keysets) {
    constexpr auto keycode_names = []() {
        auto arr = kKeycodeNames;
        arr[0] = "(Unbound)";
        return arr;
    }();

    auto table = Table<Keyset, std::tuple_size_v<Keyset>>{
        .id = "keyset_table",
        .headers = std::array{"", "", "", ""},
        .viewmodel = keysets,
        .draw_cell = [](const Keyset& keyset, size_t, size_t col) -> Action {
            auto keycode = keyset[col];
            const char* preview = keycode_names[KeycodeNormalized(keycode)];
            constexpr auto combo_flags =
                ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;
            if (!ImGui::BeginCombo("##dropdown", preview, combo_flags)) {
                return {};
            }

            Action action;
            for (uint32_t opt_keycode = 0; opt_keycode < keycode_names.size(); opt_keycode++) {
                const char* opt = keycode_names[opt_keycode];
                if (!*opt) {
                    continue;
                }
                auto is_selected = opt_keycode == keycode;
                if (ImGui::Selectable(opt, is_selected)) {
                    auto& keycode_mut = const_cast<uint32_t&>(keyset[col]);
                    action = [&keycode_mut, opt_keycode]() { keycode_mut = opt_keycode; };
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
            return action;
        },
        .draw_drag_tooltip =
            [](const Keyset& keyset) {
                auto names = std::array{
                    keycode_names[KeycodeNormalized(keyset[0])],
                    keycode_names[KeycodeNormalized(keyset[1])],
                    keycode_names[KeycodeNormalized(keyset[2])],
                    keycode_names[KeycodeNormalized(keyset[3])],
                };
                static_assert(std::tuple_size_v<decltype(names)> == std::tuple_size_v<Keyset>);
                ImGui::Text("%s+%s+%s+%s", names[0], names[1], names[2], names[3]);
            },
    };

    ImGui::SeparatorText("Keysets");
    auto action = table.Draw().first;
    if (ImGui::Button("New", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        auto& keysets_mut = const_cast<std::vector<Keyset>&>(keysets);
        action = [&keysets_mut]() { keysets_mut.emplace_back(); };
    }
    return action;
}

inline Action
DrawEquipsets(std::vector<EquipsetUI>& equipsets) {
    constexpr auto opts_template = []() {
        auto arr = std::array{"", "", ""};
        arr[static_cast<size_t>(EsItemUI::Choice::kIgnore)] = "(Ignore)";
        arr[static_cast<size_t>(EsItemUI::Choice::kUnequip)] = "(Unequip)";
        return arr;
    }();
    constexpr auto item_to_str = [](const EsItemUI& item) -> const char* {
        if (item.canonical_choice() == EsItemUI::Choice::kGear) {
            return item.gos.gear()->form().GetName();
        }
        return opts_template[static_cast<size_t>(item.canonical_choice())];
    };

    auto table = Table<EquipsetUI, kGearslots.size()>{
        .id = "equipset_table",
        .headers = equipsets.empty() ? std::array{"", "", "", ""}
                                     : std::array{"Left", "Right", "Ammo", "Voice"},
        .viewmodel = equipsets,
        .draw_cell = [](const EquipsetUI& equipset, size_t, size_t col) -> Action {
            const auto& item = equipset[col];
            const char* preview = item_to_str(item);
            constexpr auto combo_flags =
                ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;
            if (!ImGui::BeginCombo("##dropdown", preview, combo_flags)) {
                return {};
            }

            auto opts = opts_template;
            if (const auto* gear = item.gos.gear()) {
                opts[static_cast<size_t>(EsItemUI::Choice::kGear)] = gear->form().GetName();
            }

            Action action;
            for (size_t i = 0; i < opts.size(); i++) {
                const char* opt = opts[i];
                if (!*opt) {
                    continue;
                }
                auto opt_choice = static_cast<EsItemUI::Choice>(i);
                auto is_selected = opt_choice == item.canonical_choice();
                if (ImGui::Selectable(opt, is_selected)) {
                    auto& item_mut = const_cast<EsItemUI&>(item);
                    action = [&item_mut, opt_choice]() { item_mut.choice = opt_choice; };
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
            return action;
        },
        .draw_drag_tooltip =
            [](const EquipsetUI& equipset) {
                auto names = std::array{
                    item_to_str(equipset[static_cast<size_t>(Gearslot::kLeft)]),
                    item_to_str(equipset[static_cast<size_t>(Gearslot::kRight)]),
                    item_to_str(equipset[static_cast<size_t>(Gearslot::kAmmo)]),
                    item_to_str(equipset[static_cast<size_t>(Gearslot::kShout)]),
                };
                static_assert(std::tuple_size_v<decltype(names)> == kGearslots.size());
                ImGui::Text("%s, %s, %s, %s", names[0], names[1], names[2], names[3]);
            },
    };

    ImGui::SeparatorText("Equipsets");
    auto action = table.Draw().first;
    if (ImGui::Button("Add Currently Equipped", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        auto& equipset_mut = const_cast<std::vector<EquipsetUI>&>(equipsets);
        action = [&equipset_mut]() {
#ifndef ECH_UI_DEV
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                SKSE::log::error("cannot get RE::PlayerCharacter instance");
                return;
            }
            equipset_mut.push_back(EquipsetUI::From(Equipset::FromEquipped(*player)));
#else
            equipset_mut.emplace_back();
#endif
        };
    }
    return action;
}

}  // namespace internal

inline void
Draw(UI& ui) {
    const auto* main_viewport = ImGui::GetMainViewport();
    if (!main_viewport) {
        return;
    }
    const auto viewport_size = main_viewport->WorkSize;

    const auto window_initial_pos = viewport_size * ImVec2(.1f, .1f);
    const auto window_initial_size = viewport_size * ImVec2(.5f, .8f);
    const auto window_min_size = viewport_size * ImVec2(.25f, .25f);
    const auto hotkeylist_initial_size = viewport_size * ImVec2(.15f, .0f);
    const auto hotkeylist_min_size = viewport_size * ImVec2(.15f, .0f);
    constexpr auto max_size =
        ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

    // Set up main window.
    ImGui::SetNextWindowPos(window_initial_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(window_initial_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(window_min_size, max_size);
    ImGui::Begin(
        "Equipment Cycle Hotkeys", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar
    );

    internal::Action action;

    // Menu bar.
    if (ImGui::BeginMenuBar()) {
        if (auto a = internal::DrawImportMenu(ui)) {
            action = a;
        }
        if (auto a = internal::DrawExportMenu(ui)) {
            action = a;
        }
        ImGui::EndMenuBar();
    }

    // List of hotkeys.
    ImGui::SetNextWindowSizeConstraints(hotkeylist_min_size, max_size);
    ImGui::BeginChild(
        "hotkey_list", hotkeylist_initial_size, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX
    );
    if (auto a = internal::DrawHotkeyList(ui)) {
        action = a;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Hotkey details.
    ImGui::BeginChild("selected_hotkey", ImVec2(.0f, .0f));
    if (ui.selected_hotkey < ui.hotkeys_ui.size()) {
        auto& hotkey = ui.hotkeys_ui[ui.selected_hotkey];
        internal::DrawName(hotkey);

        ImGui::Dummy(ImVec2(.0f, ImGui::GetTextLineHeight()));
        if (auto a = internal::DrawKeysets(hotkey.keysets)) {
            action = a;
        }

        ImGui::Dummy(ImVec2(.0f, ImGui::GetTextLineHeight()));
        if (auto a = internal::DrawEquipsets(hotkey.equipsets)) {
            action = a;
        }
    }
    ImGui::EndChild();

    ImGui::End();
    if (action) {
        action();
    }
}

}  // namespace ui
}  // namespace ech
