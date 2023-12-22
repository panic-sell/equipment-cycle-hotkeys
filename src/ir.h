// Intermediate representations for data structures, enabling serialization/deserialization and
// UI-triggered mutations.
#pragma once

#include "equipsets.h"
#include "gear.h"
#include "hotkeys.h"
#include "keys.h"

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

template <typename K, typename Q>
struct HotkeyUI final {
    std::string name = "Hotkey";
    std::vector<K> keysets;
    std::vector<Q> equipsets;
    size_t active_equipset = 0;
};

template <typename K, typename Q>
class HotkeysUI;
template <typename Q>
HotkeysUI(const Hotkeys<Q>&) -> HotkeysUI<Keyset, Q>;
template <typename Q>
HotkeysUI(const Hotkeys<Q>&, bool) -> HotkeysUI<Keyset, Q>;

template <typename K, typename Q>
class HotkeysUI final {
  public:
    std::vector<HotkeyUI<K, Q>> hotkeys;
    size_t active_hotkey;

    explicit HotkeysUI(std::vector<HotkeyUI<K, Q>> hks = {}, size_t active_hotkey = -1)
        : hotkeys(std::move(hks)),
          active_hotkey(active_hotkey) {}

    /// If `reset_active` is true, discards the active state of hotkeys and equipsets (i.e. ensures
    /// no hotkey is active and for each hotkey, the first equipset is the active one).
    ///
    /// Persisting active state is only useful when serializing hotkey data to the SKSE co-save
    /// (i.e. persisting active status across save games).
    explicit HotkeysUI(const Hotkeys<Q>& hks, bool reset_active = true)
        : active_hotkey(hks.active()) {
        std::transform(
            hks.vec().cbegin(),
            hks.vec().cend(),
            std::back_inserter(hotkeys),
            [](const Hotkey<Q>& hotkey) {
                return HotkeyUI<Keyset, Q>{
                    .name = hotkey.name,
                    .keysets = hotkey.keysets.vec(),
                    .equipsets = hotkey.equipsets.vec(),
                    .active_equipset = hotkey.equipsets.active(),
                };
            }
        );
        if (reset_active) {
            ResetActive();
        }
    }

    /// Converts this IR object to a normal hotkeys objects.
    Hotkeys<Q>
    Into(bool reset_active = true)
    requires(std::is_same_v<K, Keyset>)
    {
        if (reset_active) {
            ResetActive();
        }
        std::vector<Hotkey<Q>> hotkeys_out;
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out),
            [](HotkeyUI<Keyset, Q>& hotkey_ir) {
                return Hotkey<Q>{
                    .name = std::move(hotkey_ir.name),
                    .keysets = Keysets(std::move(hotkey_ir.keysets)),
                    .equipsets =
                        Equipsets<Q>(std::move(hotkey_ir.equipsets), hotkey_ir.active_equipset),
                };
            }
        );
        return Hotkeys<Q>(std::move(hotkeys_out), active_hotkey);
    }

    /// Cannibalizes HotkeysUI<K, Q> to produce HotkeysUI<NewK, Q>.
    template <typename F>
    requires(std::is_invocable_v<F, const K&>)
    HotkeysUI<std::invoke_result_t<F, const K&>, Q>
    ConvertKeyset(F f) {
        using NewK = std::invoke_result_t<F, const K&>;

        auto hotkeys_out = HotkeysUI<NewK, Q>({}, active_hotkey);
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out.hotkeys),
            [&f](HotkeyUI<K, Q>& hotkey) {
                auto hotkey_out = HotkeyUI<NewK, Q>{
                    .name = std::move(hotkey.name),
                    .equipsets = std::move(hotkey.equipsets),
                };
                std::transform(
                    hotkey.keysets.cbegin(),
                    hotkey.keysets.cend(),
                    std::back_inserter(hotkey_out.keysets),
                    f
                );
                return hotkey_out;
            }
        );
        return hotkeys_out;
    }

    /// Cannibalizes HotkeysUI<K, Q> to produce HotkeysUI<K, NewQ>.
    template <typename F>
    requires(std::is_invocable_v<F, const Q&>)
    HotkeysUI<K, std::invoke_result_t<F, const Q&>>
    ConvertEquipset(F f) {
        using NewQ = std::invoke_result_t<F, const Q&>;

        auto hotkeys_out = HotkeysUI<K, NewQ>({}, active_hotkey);
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out.hotkeys),
            [&f](HotkeyUI<K, Q>& hotkey) {
                auto hotkey_out = HotkeyUI<K, NewQ>{
                    .name = std::move(hotkey.name),
                    .keysets = std::move(hotkey.keysets),
                };
                std::transform(
                    hotkey.equipsets.cbegin(),
                    hotkey.equipsets.cend(),
                    std::back_inserter(hotkey_out.equipsets),
                    f
                );
                return hotkey_out;
            }
        );
        return hotkeys_out;
    }

  private:
    /// Ensures no hotkey is active, and for each hotkey, ensures the first equipset is the active
    /// equipset.
    void
    ResetActive() {
        active_hotkey = size_t(-1);
        for (HotkeyUI<K, Q>& hotkey : hotkeys) {
            hotkey.active_equipset = 0;
        }
    }
};

}  // namespace ech
