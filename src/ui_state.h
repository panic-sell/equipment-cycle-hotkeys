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
        EquipsetUI equipset_ui;
        for (auto& item_ui : equipset_ui) {
            const auto* gos = QueryEquipset(equipset, item_ui.gos.slot());
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
        std::vector<GearOrSlot> items;
        for (const auto& item_ui : *this) {
            switch (item_ui.canonical_choice()) {
                case EsItemUI::Choice::kIgnore:
                    break;
                case EsItemUI::Choice::kGear:
                    items.push_back(item_ui.gos);
                    break;
                case EsItemUI::Choice::kUnequip:
                    items.emplace_back(item_ui.gos.slot());
                    break;
            }
        }
        return Equipset(std::move(items));
    }

  private:
    /// Finds the first item in `equipset` with the given slot.
    static const GearOrSlot*
    QueryEquipset(const Equipset& equipset, Gearslot slot) {
        const auto& v = equipset.vec();
        auto it = std::find_if(v.cbegin(), v.cend(), [=](const GearOrSlot& item) {
            return item.slot() == slot;
        });
        return it == v.cend() ? nullptr : &*it;
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
    HotkeysUI<EquipsetUI> hotkeys_ui;

    /// The hotkey being edited.
    size_t hotkey_in_focus = 0;

    /// Profile name to use when exporting.
    std::string export_name;

    /// Generic popup for (mainly error) messages.
    struct Status final {
        /// Whether to call `ImGui::OpenPopup()`.
        bool show = false;
        std::string msg;
    } status;

    UI(std::string profile_dir = fs::kProfileDir) : profile_dir_(std::move(profile_dir)) {}

    /// Intent for UI visibility. Something else has to realize this intent.
    bool
    IsActive() const {
        return active_;
    }

    /// `hotkeys` is used to populate UI data.
    void
    Activate(const Hotkeys<>& hotkeys) {
        hotkeys_ui = HotkeysUI(hotkeys).ConvertEquipset(EquipsetUI::From);
        if (hotkey_in_focus >= hotkeys_ui.size()) {
            hotkey_in_focus = 0;
        }
        saved_profiles_.reset();
        active_ = true;
    }

    /// Returns UI data converted to Hotkeys data. UI data is then discarded.
    Hotkeys<>
    Deactivate() {
        auto hotkeys = hotkeys_ui.ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
        hotkeys_ui = {};
        saved_profiles_.reset();
        active_ = false;
        return hotkeys;
    }

    /// Returns false on failing to read the profile's file.
    [[nodiscard]] bool
    ImportProfile(std::string_view profile) {
        auto p = GetProfilePath(profile);
        // clang-format off
        auto hksui = fs::ReadFile(p)
            .and_then([](std::string&& s) { return Deserialize<Hotkeys<>>(s); })
            .transform([](Hotkeys<>&& hotkeys) {
                return HotkeysUI(hotkeys).ConvertEquipset(EquipsetUI::From);
            });
        // clang-format on
        if (!hksui) {
            return false;
        }
        hotkeys_ui = std::move(*hksui);
        hotkey_in_focus = 0;
        return true;
    }

    /// Returns false on failing to write the profile's file.
    [[nodiscard]] bool
    ExportProfile() {
        auto hotkeys = HotkeysUI(hotkeys_ui).ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
        auto s = Serialize<Hotkeys<>>(hotkeys);
        NormalizeExportName();
        auto p = GetProfilePath(export_name);
        if (!fs::WriteFile(p, s)) {
            return false;
        }
        saved_profiles_.reset();
        return true;
    }

    /// 1. Removes all chars that are not `a-z`, `A-Z`, `0-9`, `-`, `_`, or ASCII 32 space.
    /// 1. Removes all leading/trailing spaces.
    /// 1. Truncates whatever is left to 32 bytes.
    void
    NormalizeExportName() {
        constexpr auto rm_invalid_chars = [](std::string& s) {
            std::erase_if(s, [](char c) {
                auto valid = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
                             || (c >= 'a' && c <= 'z') || c == ' ' || c == '_' || c == '-';
                return !valid;
            });
        };

        constexpr auto trim_space = [](std::string& s) {
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

        constexpr auto truncate_to_size = [](std::string& s, size_t size) {
            if (s.size() > size) {
                s.erase(s.begin() + size, s.end());
            }
        };

        rm_invalid_chars(export_name);
        trim_space(export_name);
        truncate_to_size(export_name, 32);
    }

    std::string
    GetProfilePath(std::string_view profile) const {
        return std::format("{}/{}{}", profile_dir_, profile, kExt);
    }

    /// Returns the list of profiles current saved to disk. This cache is refreshed on the next call
    /// to `Activate()`, `Deactivate()`, or `ExportProfile()`.
    const std::vector<std::string>&
    GetSavedProfiles() {
        if (saved_profiles_) {
            return *saved_profiles_;
        }

        saved_profiles_.emplace();
        if (!fs::ListDirToBuf(profile_dir_, *saved_profiles_)) {
            SKSE::log::error("cannot iterate list of profiles in '{}'", profile_dir_);
        }
        std::erase_if(*saved_profiles_, [](std::string_view s) {
            if (s.size() <= kExt.size()) {
                return true;
            }
            for (size_t i = 0; i < kExt.size(); i++) {
                auto ch_lower = s[s.size() - kExt.size() + i] | (1 << 5);
                if (ch_lower != kExt[i]) {
                    return true;
                }
            }
            return false;
        });
        for (auto& s : *saved_profiles_) {
            s.erase(s.end() - kExt.size(), s.end());
        }
        return *saved_profiles_;
    }

    /// Returned string_view is guaranteed to point to a std::string (meaning the underlying char
    /// array is null-terminated). Returned string_view must not outlive elements of
    /// `saved_profiles_`.
    std::optional<std::string_view>
    GetSavedProfileMatchingExportName() {
        auto export_fp = fs::PathFromStr(GetProfilePath(export_name));
        if (!export_fp) {
            return std::nullopt;
        }

        for (std::string_view profile : GetSavedProfiles()) {
            auto existing_fp = fs::PathFromStr(GetProfilePath(profile));
            if (!existing_fp) {
                continue;
            }
            std::error_code ec;
            // For this function to return true, the paths must actually exist.
            auto eq = std::filesystem::equivalent(*export_fp, *existing_fp, ec);
            if (!ec && eq) {
                return profile;
            }
        }
        return std::nullopt;
    }

    static ImVec2
    GetViewportSize() {
        static auto sz = std::optional<ImVec2>();
        if (!sz) {
            if (const auto* main_viewport = ImGui::GetMainViewport()) {
                sz.emplace(main_viewport->WorkSize);
            }
        }
        return sz ? *sz : ImVec2(800, 600);
    }

  private:
    static constexpr std::string_view kExt = ".json";

    bool active_ = false;
    std::string profile_dir_ = fs::kProfileDir;
    std::optional<std::vector<std::string>> saved_profiles_;
};

}  // namespace ech
