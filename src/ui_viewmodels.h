#pragma once

#include "equipsets.h"
#include "hotkeys.h"

namespace ech {
namespace ui {

struct EsvmItem {
    enum class Choice : uint8_t {
        /// Ignore gear slot.
        kIgnore,
        /// Equip gear.
        kGear,
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
class EquipsetVM : public std::array<EsvmItem, kGearslots.size()> {
  public:
    /// Sets all items to `Choice::kUnequip`.
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
            item_vm.choice = gos->gear() ? EsvmItem::Choice::kGear : EsvmItem::Choice::kUnequip;
        }
        return esvm;
    }

    Equipset
    To() const {
        std::vector<GearOrSlot> items;
        for (const auto& item_vm : *this) {
            switch (item_vm.canonical_choice()) {
                case EsvmItem::Choice::kIgnore:
                    break;
                case EsvmItem::Choice::kGear:
                    items.push_back(item_vm.gos);
                    break;
                case EsvmItem::Choice::kUnequip:
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
struct HotkeyVM {
    std::string name;
    std::vector<Keyset> keysets;
    std::vector<T> equipsets;

    bool operator==(const HotkeyVM&) const = default;
};

template <typename T = EquipsetVM>
class HotkeysVM : public std::vector<HotkeyVM<T>> {
  public:
    using std::vector<HotkeyVM<T>>::vector;

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
            hotkeys_vm.push_back(std::move(hk_vm));
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
        for (const HotkeyVM<T>& hk_vm : *this) {
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
};

}  // namespace ui
}  // namespace ech
