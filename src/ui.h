#pragma once

#include "ui_viewmodels.h"

namespace ech {
namespace ui {
namespace internal {

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
    const auto cols = last_col + 1;
    const auto rows = row_entities.size();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2, 4));
    constexpr auto table_flags = ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_BordersInnerH;
    if (!ImGui::BeginTable(table_id, cols, table_flags)) {
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
            ImGui::PushID(row * cols + col);
            draw_cell(row_entity, row, col);
            ImGui::PopID();
        }

        // Control buttons.
        {
            auto draw_drag = [&]() { draw_drag_tooltip(row_entity); };
            ImGui::TableSetColumnIndex(last_col);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32_BLACK_TRANS);
            ImGui::PushID(row * cols + last_col);

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

    auto draw_drag_tooltip = [](const HotkeyVM<>& hotkey_vm) {
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
    // if (ImGui::Button("Clone Selected", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
    //     hotkeys_vm.insert(hotkeys_vm.begin() + selected + 1, hotkeys_vm[selected]);
    //     selected++;
    // }
}

inline void
DrawKeysets(std::vector<Keyset>& keysets) {
    constexpr auto headers = std::array{"", "", "", ""};

    auto draw_dropdown = [](Keyset& keyset, int, int col) {
        auto& keycode = keyset[col];
        const char* preview = kKeycodeNamesForUI[KeycodeNormalize(keycode)];
        constexpr auto combo_flags = ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;
        if (!ImGui::BeginCombo("##dropdown", preview, combo_flags)) {
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

    auto draw_drag_tooltip = [](const Keyset& keyset) {
        auto names = std::array{
            kKeycodeNamesForUI[KeycodeNormalize(keyset[0])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[1])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[2])],
            kKeycodeNamesForUI[KeycodeNormalize(keyset[3])],
        };
        static_assert(std::tuple_size_v<decltype(names)> == std::tuple_size_v<Keyset>);
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
        const char* preview = EsvmItemToCStr(esvm_item);
        constexpr auto combo_flags = ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton;
        if (!ImGui::BeginCombo("##dropdown", preview, combo_flags)) {
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
        auto names = std::array{
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kLeft)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kRight)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kAmmo)]),
            EsvmItemToCStr(equipset_vm[static_cast<size_t>(Gearslot::kShout)]),
        };
        static_assert(std::tuple_size_v<decltype(names)> == kGearslots.size());
        ImGui::Text("%s, %s, %s, %s", names[0], names[1], names[2], names[3]);
    };

    ImGui::SeparatorText("Equipsets");
    DrawTable<EquipsetVM>(
        "equipset_table", equipsets_vm, headers, draw_dropdown, draw_drag_tooltip
    );
    if (ImGui::Button("Add Currently Equipped", ImGui::GetContentRegionAvail() * ImVec2(1, 0))) {
        equipsets_vm.emplace_back();
    }
}

}  // namespace internal

inline void
SetStyle() {
    // auto& style = ImGui::GetStyle();
    // style.CellPadding = ImVec2(2, 4);
}

struct Dims {
    ImVec2 max_size = {FLT_MAX, FLT_MAX};
    ImVec2 window_initial_pos = {.1f, .1f};
    ImVec2 window_initial_size = {.5f, .8f};
    ImVec2 window_min_size = {.25f, .25f};
    ImVec2 hklist_initial_size = {.15f, 0};
    ImVec2 hklist_min_size = {.15f, 0};

    explicit Dims(const ImVec2& viewport_size) {
        window_initial_pos *= viewport_size;
        window_initial_size *= viewport_size;
        window_min_size *= viewport_size;
        hklist_initial_size *= viewport_size;
        hklist_min_size *= viewport_size;
    }
};

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

inline void
Draw(HotkeysVM<>& hotkeys_vm, int& selected) {
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (!main_viewport) {
        return;
    }
    const auto dims = Dims(main_viewport->WorkSize);
    ImGui::SetNextWindowPos(dims.window_initial_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(dims.window_initial_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(dims.window_min_size, dims.max_size);
    constexpr auto window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar
                                  | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Equipment Cycle Hotkeys", nullptr, window_flags);

    static std::vector<std::string> filenames;
    auto import_callback = [&](std::string_view fn) {

    };
    auto export_callback = [&](std::string_view fn) {

    };
    DrawMenuBar(filenames, import_callback, export_callback);

    ImGui::SetNextWindowSizeConstraints(dims.hklist_min_size, dims.max_size);
    ImGui::BeginChild(
        "hotkey_list",
        dims.hklist_initial_size,
        ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX,
        ImGuiWindowFlags_NoSavedSettings
    );
    internal::DrawHotkeyList(hotkeys_vm, selected);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("selected_hotkey", ImVec2(0, 0));
    if (selected >= 0 && selected < hotkeys_vm.size()) {
        auto& hotkey_vm = hotkeys_vm[selected];
        ImGui::InputTextWithHint("Hotkey Name", "(Unnamed)", &hotkey_vm.name);
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        internal::DrawKeysets(hotkey_vm.keysets);
        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        internal::DrawEquipsets(hotkey_vm.equipsets);
    }
    ImGui::EndChild();

    ImGui::End();
}

class UI {
  public:
  private:
};

}  // namespace ui
}  // namespace ech
