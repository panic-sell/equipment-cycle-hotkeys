#pragma once

#include "equipsets.h"
#include "hotkeys.h"
#include "keys.h"

namespace ech {
namespace ui {

struct UISettings {
    static constexpr auto kColors = std::array{"dark"sv, "light"sv, "classic"sv};

    Keysets toggle_keysets;
    float font_scale = 1.f;
    std::string_view color = kColors[0];
};

struct EsItemVM final {
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

/// Equipset view model.
///
/// Invariants:
/// - `(*this)[i].slot() == static_cast<Gearslot>(i)`
class EquipsetVM final : public std::array<EsItemVM, kGearslots.size()> {
  public:
    EquipsetVM() {
        for (auto slot : kGearslots) {
            (*this)[static_cast<size_t>(slot)].gos = slot;
        }
    }

    static EquipsetVM
    From(const Equipset& equipset) {
        EquipsetVM esvm;
        for (auto& item_vm : esvm) {
            auto* gos = QueryEquipset(equipset, item_vm.gos.slot());
            if (!gos) {
                continue;
            }
            item_vm.gos = *gos;
            item_vm.choice = gos->gear() ? EsItemVM::Choice::kGear : EsItemVM::Choice::kUnequip;
        }
        return esvm;
    }

    Equipset
    To() const {
        std::vector<GearOrSlot> items;
        for (const auto& item_vm : *this) {
            switch (item_vm.canonical_choice()) {
                case EsItemVM::Choice::kIgnore:
                    break;
                case EsItemVM::Choice::kGear:
                    items.push_back(item_vm.gos);
                    break;
                case EsItemVM::Choice::kUnequip:
                    items.emplace_back(item_vm.gos.slot());
                    break;
            }
        }
        return Equipset(std::move(items));
    }

  private:
    /// Finds the first item in `equipset` with the given slot.
    static const GearOrSlot*
    QueryEquipset(const Equipset& equipset, Gearslot slot) {
        auto& v = equipset.vec();
        auto it = std::find_if(v.cbegin(), v.cend(), [=](const GearOrSlot& item) {
            return item.slot() == slot;
        });
        return it == v.cend() ? nullptr : &*it;
    }
};

template <typename T = EquipsetVM>
struct HotkeyVM final {
    std::string name = "Hotkey";
    std::vector<Keyset> keysets;
    std::vector<T> equipsets;
    size_t active_equipset = 0;
};

template <typename T = EquipsetVM>
struct HotkeysVM final {
    std::vector<HotkeyVM<T>> vec;
    size_t active = std::numeric_limits<size_t>::max();

    static HotkeysVM<EquipsetVM>
    From(const Hotkeys<Equipset>& hotkeys)
    requires(std::is_same_v<T, EquipsetVM>)
    {
        return From<Equipset>(hotkeys, EquipsetVM::From);
    }

    template <typename U>
    static HotkeysVM<T>
    From(const Hotkeys<U>& hotkeys, std::function<T(const U&)> f) {
        HotkeysVM<T> hotkeys_vm;
        for (const Hotkey<U>& hk : hotkeys.vec()) {
            std::vector<T> equipsets_vm;
            std::transform(
                hk.equipsets.vec().cbegin(),
                hk.equipsets.vec().cend(),
                std::back_inserter(equipsets_vm),
                f
            );

            auto hk_vm = HotkeyVM<T>{
                .name = hk.name,
                .keysets = hk.keysets.vec(),
                .equipsets = std::move(equipsets_vm),
            };
            hotkeys_vm.vec.push_back(std::move(hk_vm));
        }
        return hotkeys_vm;
    }

    Hotkeys<Equipset>
    To() const
    requires(std::is_same_v<T, EquipsetVM>)
    {
        return To<Equipset>(&EquipsetVM::To);
    }

    template <typename U>
    Hotkeys<U>
    To(std::function<U(const T&)> f) const {
        std::vector<Hotkey<U>> hotkeys;
        for (const HotkeyVM<T>& hk_vm : vec) {
            std::vector<U> equipsets;
            std::transform(
                hk_vm.equipsets.cbegin(), hk_vm.equipsets.cend(), std::back_inserter(equipsets), f
            );

            auto hk = Hotkey<U>{
                .name = hk_vm.name,
                .keysets = Keysets(hk_vm.keysets),
                .equipsets = Equipsets<U>(std::move(equipsets)),
            };
            hotkeys.push_back(std::move(hk));
        }
        return Hotkeys(std::move(hotkeys));
    }

    void
    ResetActive() {
        active = std::numeric_limits<size_t>::max();
        for (HotkeyVM<T>& hotkey : vec) {
            hotkey.active_equipset = 0;
        }
    }
};

}  // namespace ui
}  // namespace ech
