#pragma once

#include "fs.h"
#include "ir.h"
#include "keys.h"
#include "serde.h"

namespace ech {
namespace ui {

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

struct Context final {
    std::filesystem::path profile_dir;
    std::vector<std::string> profile_cache;
    std::string export_name_buf;
    HotkeysIR<Keyset, EquipsetUI> hotkeys_ui;
    /// What's selected in UI. Has nothing to do with whether a hotkey is "active" during gameplay.
    size_t selected_hotkey_index = 0;

    /// Returns nullptr if `selected_hotkey_index` is out of bounds.
    HotkeyIR<Keyset, EquipsetUI>*
    selected_hotkey() {
        if (selected_hotkey_index >= hotkeys_ui.hotkeys.size()) {
            return nullptr;
        }
        return &hotkeys_ui.hotkeys[selected_hotkey_index];
    }

    void
    ReloadProfileCache() {
        profile_cache.clear();
        if (!fs::ListDirectoryToBuffer(profile_dir, profile_cache)) {
            // TODO: log
        }
    }
};

inline Action
DrawImportMenu(Context& ctx) {
    constexpr auto try_parse_profile =
        [](const std::filesystem::path&) -> std::optional<HotkeysIR<Keyset, EquipsetUI>> {
        // TODO: implement
        return std::nullopt;
    };

    if (!ImGui::BeginMenu("Import")) {
        return {};
    }

    Action action;
    if (ctx.profile_cache.empty()) {
        ImGui::Text("No profiles to import.");
    } else {
        for (const auto& profile : ctx.profile_cache) {
            if (!ImGui::MenuItem(profile.c_str())) {
                continue;
            }
            action = [&ctx, &profile]() {
                auto hotkeys_ui = try_parse_profile(ctx.profile_dir / profile);
                ctx.ReloadProfileCache();
                if (!hotkeys_ui) {
                    // TODO: log
                    return;
                }
                ctx.hotkeys_ui = std::move(*hotkeys_ui);
                ctx.selected_hotkey_index = 0;
            };
        }
    }

    ImGui::EndMenu();
    return action;
}

inline Action
DrawExportMenu(Context& ctx) {
    auto confirm_export = false;
    if (ImGui::BeginMenu("Export")) {
        ImGui::InputTextWithHint("##new_profile", "Enter profile name...", &ctx.export_name_buf);
        if (ImGui::Button("Export Profile")) {
            confirm_export = !ctx.export_name_buf.empty();
        }
        ImGui::EndMenu();
    }

    Action action;
    if (confirm_export) {
        ImGui::OpenPopup("confirm_export");
    }
    if (ImGui::BeginPopup("confirm_export")) {
        auto in_cache =
            std::find(ctx.profile_cache.cbegin(), ctx.profile_cache.cend(), ctx.export_name_buf)
            == ctx.profile_cache.cend();
        const char* msg = in_cache ? "Create new profile '%s'?"
                                   : "Overwrite existing profile '%s'?";
        ImGui::Text(msg, ctx.export_name_buf.c_str());
        if (ImGui::Button("Yes")) {
            action = [&ctx]() {
                auto hotkeys_real =
                    HotkeysIR(ctx.hotkeys_ui).ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
                // TODO: convert to json
                std::error_code ec;
                std::filesystem::create_directories(ctx.profile_dir, ec);
                if (ec) {
                    // TODO: log error
                    return;
                }
                // TODO: implement export
                ctx.ReloadProfileCache();
            };
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
DrawSettingsMenu() {
    if (!ImGui::BeginMenu("Settings")) {
        return {};
    }

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

    ImGui::EndMenu();
    return {};
}

inline Action
DrawHotkeyList(Context& ctx) {
    auto table = Table<HotkeyIR<Keyset, EquipsetUI>, 1>{
        .id = "hotkeys_list",
        .headers = std::array{""},
        .viewmodel = ctx.hotkeys_ui.hotkeys,
        .draw_cell =
            [&ctx](const HotkeyIR<Keyset, EquipsetUI>& hotkey, size_t row, size_t) -> Action {
            const char* label = hotkey.name.empty() ? "(Unnamed)" : hotkey.name.c_str();
            if (!ImGui::RadioButton(label, row == ctx.selected_hotkey_index)) {
                return {};
            }
            return [&ctx, row]() { ctx.selected_hotkey_index = row; };
        },
        .draw_drag_tooltip = [](const HotkeyIR<Keyset, EquipsetUI>& hotkey
                             ) { ImGui::Text("%s", hotkey.name.c_str()); },
    };

    auto atrc = table.Draw();
    if (ImGui::Button("New", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        auto a = [&ctx]() {
            ctx.hotkeys_ui.hotkeys.emplace_back();
            // Adding a new hotkey selects that hotkey.
            ctx.selected_hotkey_index = ctx.hotkeys_ui.hotkeys.size() - 1;
        };
        atrc = {std::move(a), {}};
    }
    if (!atrc.first) {
        return {};
    }

    return [atrc, &ctx]() {
        const auto& [a, trc] = atrc;
        if (trc.remove < ctx.hotkeys_ui.hotkeys.size()) {
            // If the selected hotkey is below the removed hotkey, then move selection upward.
            if (trc.remove < ctx.selected_hotkey_index) {
                ctx.selected_hotkey_index--;
            }
        } else if (
            trc.drag_source < ctx.hotkeys_ui.hotkeys.size()
            && trc.drag_target < ctx.hotkeys_ui.hotkeys.size()
        ) {
            // Select the row that was dragged.
            ctx.selected_hotkey_index = trc.drag_target;
        }
        a();
        if (ctx.selected_hotkey_index >= ctx.hotkeys_ui.hotkeys.size()
            && ctx.selected_hotkey_index > 0) {
            ctx.selected_hotkey_index--;
        }
    };
}

inline void
DrawName(HotkeyIR<Keyset, EquipsetUI>& hotkey) {
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
            constexpr auto combo_flags = ImGuiComboFlags_HeightLarge
                                         | ImGuiComboFlags_NoArrowButton;
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
            constexpr auto combo_flags = ImGuiComboFlags_HeightLarge
                                         | ImGuiComboFlags_NoArrowButton;
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
#ifdef ECH_UI_DEV_MODE
            equipset_mut.emplace_back();
#else
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                return;
            }
            equipset_mut.push_back(EquipsetUI::From(Equipset::FromEquipped(*player)));
#endif
        };
    }
    return action;
}

inline void
Draw() {
    static auto ctx = []() {
        auto c = Context{
            .profile_dir = fs::kProfileDir,
            .hotkeys_ui = HotkeysIR(std::vector<HotkeyIR<Keyset, EquipsetUI>>{
                {
                    .name = "asdf",
                    .keysets{
                        {1, 2, 3, 4},
                        {5, 0, 45, 104},
                        {7, 0, 0, 0},
                        {4, 3, 2, 1},
                        {0, 20, 19, 18},
                        {0},
                    },
                    .equipsets{
                        {},
                        {},
                        {},
                        {},
                    },
                },
            })
        };
        c.ReloadProfileCache();
        return c;
    }();

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

    const auto* main_viewport = ImGui::GetMainViewport();
    if (!main_viewport) {
        return;
    }
    const auto dims = Dims(main_viewport->WorkSize);

    // Set up main window.
    ImGui::SetNextWindowPos(dims.window_initial_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(dims.window_initial_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(dims.window_min_size, dims.max_size);
    ImGui::Begin(
        "Equipment Cycle Hotkeys", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar
    );

    Action action;

    // Menu bar.
    if (ImGui::BeginMenuBar()) {
        if (auto a = DrawImportMenu(ctx)) {
            action = a;
        }
        if (auto a = DrawExportMenu(ctx)) {
            action = a;
        }
        if (auto a = DrawSettingsMenu()) {
            action = a;
        }
        ImGui::EndMenuBar();
    }

    // List of hotkeys.
    ImGui::SetNextWindowSizeConstraints(dims.hklist_min_size, dims.max_size);
    ImGui::BeginChild(
        "hotkey_list", dims.hklist_initial_size, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX
    );
    if (auto a = DrawHotkeyList(ctx)) {
        action = a;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Hotkey details.
    ImGui::BeginChild("selected_hotkey", ImVec2(0, 0));
    if (ctx.selected_hotkey_index < ctx.hotkeys_ui.hotkeys.size()) {
        auto& hotkey = ctx.hotkeys_ui.hotkeys[ctx.selected_hotkey_index];
        DrawName(hotkey);

        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        if (auto a = DrawKeysets(hotkey.keysets)) {
            action = a;
        }

        ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
        if (auto a = DrawEquipsets(hotkey.equipsets)) {
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
