// Intermediate representations for data structures, enabling UI-triggered mutations.
#pragma once

#include "equipsets.h"
#include "fs.h"
#include "gear.h"
#include "hotkeys.h"
#include "keys.h"
#include "serde.h"

namespace ech {

struct EsItemUI final {
    enum class Choice {
        /// Equip gear.
        kGear,
        /// Ignore gear slot.
        kIgnore,
        /// Unequip gear slot.
        kUnequip,
    };

    GearOrSlot gos = static_cast<Gearslot>(0);
    Choice choice = Choice::kIgnore;

    /// If this returns `kGear`, then `gos` is guaranteed to contain a `Gear` object.
    Choice
    canonical_choice() const {
        if (choice == Choice::kGear && !gos.gear()) {
            return Choice::kIgnore;
        }
        return choice;
    }
};

/// Equipset UI view model.
///
/// Invariants:
/// - `(*this)[i].slot() == static_cast<Gearslot>(i)`
class EquipsetUI final : public std::array<EsItemUI, kGearslots.size()> {
  public:
    EquipsetUI() {
        for (auto slot : kGearslots) {
            (*this)[static_cast<size_t>(slot)].gos = slot;
        }
    }

    static EquipsetUI
    From(const Equipset& equipset) {
        auto equipset_ui = EquipsetUI();
        for (auto& item_ui : equipset_ui) {
            const auto* gos = equipset.Get(item_ui.gos.slot());
            if (!gos) {
                continue;
            }
            item_ui.gos = *gos;
            item_ui.choice = gos->gear() ? EsItemUI::Choice::kGear : EsItemUI::Choice::kUnequip;
        }
        return equipset_ui;
    }

    Equipset
    To() const {
        auto items = std::vector<GearOrSlot>();
        for (const auto& item_ui : *this) {
            switch (item_ui.canonical_choice()) {
                case EsItemUI::Choice::kIgnore:
                    break;
                case EsItemUI::Choice::kGear:
                    items.push_back(item_ui.gos);
                    break;
                case EsItemUI::Choice::kUnequip:
                    items.push_back(item_ui.gos.slot());
                    break;
            }
        }
        return Equipset(std::move(items));
    }
};

template <typename Q>
struct HotkeyUI final {
    std::string name;
    std::vector<Keyset> keysets;
    std::vector<Q> equipsets;
};

template <typename Q>
class HotkeysUI final : public std::vector<HotkeyUI<Q>> {
  public:
    using std::vector<HotkeyUI<Q>>::vector;

    explicit HotkeysUI(const Hotkeys<Q>& hks) {
        std::transform(
            hks.vec().cbegin(),
            hks.vec().cend(),
            std::back_inserter(*this),
            [](const Hotkey<Q>& hotkey) {
                return HotkeyUI<Q>{
                    .name = hotkey.name,
                    .keysets = hotkey.keysets.vec(),
                    .equipsets = hotkey.equipsets.vec(),
                };
            }
        );
    }

    /// Converts this object to a normal hotkeys object.
    Hotkeys<Q>
    Into() {
        std::vector<Hotkey<Q>> hotkeys_out;

        std::transform(
            this->begin(),
            this->end(),
            std::back_inserter(hotkeys_out),
            [](HotkeyUI<Q>& hotkey_ui) {
                return Hotkey<Q>{
                    .name = std::move(hotkey_ui.name),
                    .keysets = Keysets(std::move(hotkey_ui.keysets)),
                    .equipsets = Equipsets<Q>(std::move(hotkey_ui.equipsets)),
                };
            }
        );
        return Hotkeys<Q>(std::move(hotkeys_out));
    }

    /// Cannibalizes HotkeysUI<Q> to produce HotkeysUI<NewQ>.
    template <typename F>
    requires(std::is_invocable_v<F, const Q&>)
    HotkeysUI<std::invoke_result_t<F, const Q&>>
    ConvertEquipset(F f) {
        using NewQ = std::invoke_result_t<F, const Q&>;

        HotkeysUI<NewQ> hotkeys_new;
        std::transform(
            this->begin(),
            this->end(),
            std::back_inserter(hotkeys_new),
            [&f](HotkeyUI<Q>& hotkey) {
                auto hotkey_new = HotkeyUI<NewQ>{
                    .name = std::move(hotkey.name),
                    .keysets = std::move(hotkey.keysets),
                };
                std::transform(
                    hotkey.equipsets.cbegin(),
                    hotkey.equipsets.cend(),
                    std::back_inserter(hotkey_new.equipsets),
                    f
                );
                return hotkey_new;
            }
        );
        return hotkeys_new;
    }
};

/// Main container for all UI-related state.
class UI final {
  public:
    /// Popup for (mainly error) messages.
    struct Status final {
        bool should_call_imgui_open_popup = false;
        std::string msg;

        void
        SetMsg(std::string s) {
            should_call_imgui_open_popup = true;
            msg = std::move(s);
        }
    };

    /// State that gets created/destroyed on UI activation/deactivation. The parent UI class holds
    /// state that persists across UI activations.
    class StateEphemeral final {
      public:
        HotkeysUI<EquipsetUI> hotkeys_ui;
        Status status;
        std::string import_name;
        /// This becomes false when user clicks the "close" widget on the UI window.
        bool imgui_begin_p_open = true;

      private:
        friend class UI;

        std::optional<std::vector<std::string>> saved_profiles_;
    };

    static constexpr std::string_view kProfileExt = ".json";

    /// UI active state is indicated by the truthiness of this value.
    std::optional<StateEphemeral> eph;
    size_t hotkey_in_focus = 0;
    std::string export_name;
    const std::string profile_dir;

    UI(std::string profile_dir = fs::kProfileDir) : profile_dir(std::move(profile_dir)) {}

    /// `hotkeys` is used to populate UI data.
    void
    Activate(const Hotkeys<>* hotkeys = nullptr) {
        eph.emplace();
        if (hotkeys) {
            eph->hotkeys_ui = HotkeysUI(*hotkeys).ConvertEquipset(EquipsetUI::From);
        }
        if (hotkey_in_focus >= eph->hotkeys_ui.size()) {
            hotkey_in_focus = 0;
        }
#ifndef ECH_TEST
        ImGui::GetIO().MouseDrawCursor = true;
#endif
    }

    /// Syncs `hotkeys` with UI data (if `hotkeys` is non-null), then destroys all ephemeral data.
    void
    Deactivate(Hotkeys<>* hotkeys = nullptr) {
#ifndef ECH_TEST
        ImGui::GetIO().MouseDrawCursor = false;
#endif
        if (!eph) {
            return;
        }
        if (hotkeys) {
            auto new_hotkeys = eph->hotkeys_ui.ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
            if (!hotkeys->StructurallyEquals(new_hotkeys)) {
                // This also resets selected hotkey/equipset state.
                *hotkeys = std::move(new_hotkeys);
                SKSE::log::debug("active hotkeys modified");
            }
        }
        eph.reset();
    }

    /// Returns false on failing to read the profile's file.
    ///
    /// No-op and returns true if UI is not active.
    [[nodiscard]] bool
    ImportProfile() {
        if (!eph) {
            return true;
        }

        auto fp = GetProfilePath(eph->import_name);
        eph->saved_profiles_.reset();
        // clang-format off
        auto hksui = fs::ReadFile(fp)
            .and_then([](std::string&& s) { return Deserialize<Hotkeys<>>(s); })
            .transform([](Hotkeys<>&& hotkeys) {
                return HotkeysUI(hotkeys).ConvertEquipset(EquipsetUI::From);
            });
        // clang-format on
        if (!hksui) {
            return false;
        }
        eph->hotkeys_ui = std::move(*hksui);
        hotkey_in_focus = 0;
        return true;
    }

    /// Returns false on failing to write the profile's file.
    ///
    /// No-op and returns true if UI is not active.
    [[nodiscard]] bool
    ExportProfile() {
        if (!eph) {
            return true;
        }

        auto hotkeys =
            HotkeysUI(eph->hotkeys_ui).ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
        auto s = Serialize<Hotkeys<>>(hotkeys);
        auto fp = GetProfilePath(GetNormalizedExportName());
        eph->saved_profiles_.reset();
        if (!fs::WriteFile(fp, s)) {
            return false;
        }
        return true;
    }

    /// Returns false on failing to remove the profile's file.
    ///
    /// No-op and returns true if UI is not active.
    [[nodiscard]] bool
    DeleteProfile() {
        if (!eph) {
            return true;
        }
        auto fp = GetProfilePath(export_name);
        eph->saved_profiles_.reset();
        if (!fs::RemoveFile(fp)) {
            return false;
        }
        return true;
    }

    /// Returns a reference to export_name after:
    /// 1. Removing all chars that are not `a-z`, `A-Z`, `0-9`, `-`, `_`, or ASCII 32 space.
    /// 1. Removing all leading/trailing spaces.
    /// 1. Truncating whatever is left to 32 bytes.
    std::string&
    GetNormalizedExportName() {
        constexpr auto rm_invalid_chars = [](std::string& s) -> void {
            std::erase_if(s, [](char c) {
                auto valid = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
                             || (c >= 'a' && c <= 'z') || c == ' ' || c == '_' || c == '-';
                return !valid;
            });
        };

        constexpr auto trim_space = [](std::string& s) -> void {
            auto last = s.find_last_not_of(' ');
            if (last == std::string::npos) {
                s.clear();
                return;
            }
            s.erase(s.begin() + last + 1, s.end());

            auto first = s.find_first_not_of(' ');
            if (first == std::string::npos) {
                s.clear();
                return;
            }
            s.erase(s.begin(), s.begin() + first);
        };

        constexpr auto truncate_to_size = [](std::string& s, size_t size) -> void {
            if (s.size() > size) {
                s.erase(s.begin() + size, s.end());
            }
        };

        rm_invalid_chars(export_name);
        trim_space(export_name);
        truncate_to_size(export_name, 32);
        return export_name;
    }

    std::string
    GetProfilePath(std::string_view profile) const {
        return std::format("{}/{}{}", profile_dir, profile, kProfileExt);
    }

    /// Returns the list of profiles current saved to disk. This cache is refreshed on the next call
    /// to `ImportProfile()`, `ExportProfile()`, or `DeleteProfile()`.
    ///
    /// Returns an empty vec if UI is not active.
    const std::vector<std::string>&
    GetSavedProfiles() {
        static const auto empty = std::vector<std::string>();
        if (!eph) {
            return empty;
        }

        if (eph->saved_profiles_) {
            return *eph->saved_profiles_;
        }

        auto v = std::vector<std::string>();
        if (!fs::ListDirToBuf(profile_dir, v)) {
            SKSE::log::error("cannot iterate list of profiles in '{}'", profile_dir);
        }
        std::erase_if(v, [](std::string_view s) {
            if (s.size() <= kProfileExt.size()) {
                return true;
            }
            for (size_t i = 0; i < kProfileExt.size(); i++) {
                auto ch_lower = s[s.size() - kProfileExt.size() + i] | (1 << 5);
                if (ch_lower != kProfileExt[i]) {
                    return true;
                }
            }
            return false;
        });
        for (auto& s : v) {
            s.erase(s.end() - kProfileExt.size(), s.end());
        }
        eph->saved_profiles_.emplace(std::move(v));
        return *eph->saved_profiles_;
    }

    /// Returned string must not outlive elements of `saved_profiles_`.
    const std::string*
    GetSavedProfileMatching(std::string_view name) {
        auto fp = fs::PathFromStr(GetProfilePath(name));
        if (!fp) {
            return nullptr;
        }

        for (const auto& profile : GetSavedProfiles()) {
            auto existing_fp = fs::PathFromStr(GetProfilePath(profile));
            if (!existing_fp) {
                continue;
            }
            auto ec = std::error_code();
            // For this function to return true, the paths must actually exist.
            auto eq = std::filesystem::equivalent(*fp, *existing_fp, ec);
            if (!ec && eq) {
                return &profile;
            }
        }
        return nullptr;
    }
};

}  // namespace ech
