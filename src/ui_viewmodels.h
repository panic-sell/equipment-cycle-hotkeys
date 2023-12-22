// Intermediate representations for data structures, enabling UI-triggered mutations.
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

}  // namespace ech
