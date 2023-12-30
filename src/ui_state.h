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
    std::string name = "Hotkey";
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
    // Created/destroyed through activation state changes.
    std::vector<std::string> profile_cache;
    HotkeysUI<EquipsetUI> hotkeys_ui;

    // Persists between activation state changes.
    size_t selected_hotkey = 0;
    std::string export_profile;

    UI(std::string profile_dir = fs::kProfileDir) : profile_dir_(std::move(profile_dir)) {
        while (profile_dir_.ends_with('/')) {
            profile_dir_.pop_back();
        }
    }

    /// Intent for UI visibility. Something else has to realize this intent.
    bool
    IsActive() const {
        return active_;
    }

    /// `hotkeys` is used to populate UI data.
    void
    Activate(const Hotkeys<>& hotkeys) {
        hotkeys_ui = HotkeysUI(hotkeys).ConvertEquipset(EquipsetUI::From);
        if (selected_hotkey >= hotkeys_ui.size()) {
            selected_hotkey = 0;
        }
        ReloadProfileCache();
        active_ = true;
    }

    /// Returns UI data converted to Hotkeys data. UI data is then discarded.
    Hotkeys<>
    Deactivate() {
        auto hotkeys = hotkeys_ui.ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
        hotkeys_ui = {};
        profile_cache.clear();
        active_ = false;
        return hotkeys;
    }

    void
    ImportProfile(std::string_view profile) {
        auto p = GetProfilePath(profile);
        // clang-format off
        auto hksui = fs::Read(p)
            .and_then([](std::string&& s) { return Deserialize<Hotkeys<>>(s); })
            .transform([](Hotkeys<>&& hotkeys) {
                return HotkeysUI(hotkeys).ConvertEquipset(EquipsetUI::From);
            });
        // clang-format on
        if (!hksui) {
            SKSE::log::error("cannot read '{}'", p);
            return;
        }
        hotkeys_ui = std::move(*hksui);
        ReloadProfileCache();
        selected_hotkey = 0;
    }

    void
    ExportProfile() {
        auto hotkeys = HotkeysUI(hotkeys_ui).ConvertEquipset(std::mem_fn(&EquipsetUI::To)).Into();
        auto s = Serialize<Hotkeys<>>(hotkeys);
        NormalizeExportProfile();
        auto p = GetProfilePath(export_profile);
        if (!fs::Write(p, s)) {
            SKSE::log::error("cannot write '{}'", p);
            return;
        }
        ReloadProfileCache();
    }

    /// 1. Removes all chars that are not `a-z`, `A-Z`, `0-9`, `-`, `_`, or ASCII 32 space.
    /// 1. Removes all leading/trailing spaces.
    /// 1. Truncates whatever is left to 32 bytes.
    void
    NormalizeExportProfile() {
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

        rm_invalid_chars(export_profile);
        trim_space(export_profile);
        truncate_to_size(export_profile, 32);
    }

    const std::string*
    FindCachedProfileMatchingExportProfile() const {
        auto export_fp = fs::PathFromStr(GetProfilePath(export_profile));
        if (!export_fp) {
            return nullptr;
        }

        for (const auto& cached : profile_cache) {
            auto existing_fp = fs::PathFromStr(GetProfilePath(cached));
            if (!existing_fp) {
                continue;
            }
            std::error_code ec;
            auto eq = std::filesystem::equivalent(*export_fp, *existing_fp, ec);
            if (!ec && eq) {
                return &cached;
            }
        }
        return nullptr;
    }

  private:
    static constexpr std::string_view kExt = ".json";

    void
    ReloadProfileCache() {
        profile_cache.clear();
        if (!fs::ListDirectoryToBuffer(profile_dir_, profile_cache)) {
            SKSE::log::error("cannot iterate '{}'", profile_dir_);
        }
        std::erase_if(profile_cache, [](std::string_view s) {
            return s == kExt || !s.ends_with(kExt);
        });
        for (auto& s : profile_cache) {
            s.erase(s.end() - kExt.size(), s.end());
        }
    }

    std::string
    GetProfilePath(std::string_view profile) const {
        return std::format("{}/{}{}", profile_dir_, profile, kExt);
    }

    bool active_ = false;
    std::string profile_dir_ = fs::kProfileDir;
};

}  // namespace ech
