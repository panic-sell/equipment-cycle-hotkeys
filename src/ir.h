// Intermediate representations for data structures, enabling serialization/deserialization and
// UI-triggered mutations.
#pragma once

#include "equipsets.h"
#include "gear.h"
#include "hotkeys.h"
#include "keys.h"

namespace ech {

class KeysetSer final : public std::vector<std::string> {
  public:
    static KeysetSer
    From(const Keyset& keyset) {
        KeysetSer v;
        for (auto c : keyset) {
            if (KeycodeIsValid(c)) {
                v.emplace_back(KeycodeName(c));
            }
        }
        return v;
    }

    Keyset
    To() const {
        auto keyset = Keyset{0};
        auto sz = std::min(size(), keyset.size());
        for (size_t i = 0; i < sz; i++) {
            auto& name = (*this)[i];
            keyset[i] = KeycodeFromName(name);
        }
        KeysetNormalize(keyset);
        return keyset;
    }
};

struct EsItemSer final {
    uint8_t slot = 0;
    bool unequip = false;

    std::string modname = "";
    RE::FormID id = 0;
    float extra_health = std::numeric_limits<float>::quiet_NaN();
    std::string extra_ench_modname = "";
    RE::FormID extra_ench_id = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        EsItemSer, slot, unequip, modname, id, extra_health, extra_ench_modname, extra_ench_id
    );

    static EsItemSer
    From(const GearOrSlot& gos) {
        auto item = EsItemSer{.slot = std::to_underlying(gos.slot())};
        if (!gos.gear()) {
            item.unequip = true;
            return item;
        }
        const auto& [modname, id] = tes_util::GetNamedFormID(gos.gear()->form());
        item.modname = std::string(modname);
        item.id = id;
        item.extra_health = gos.gear()->extra_health();
        if (auto* extra_ench = gos.gear()->extra_ench()) {
            const auto& [ee_modname, ee_id] = tes_util::GetNamedFormID(*extra_ench);
            item.extra_ench_modname = ee_modname;
            item.extra_ench_id = ee_id;
        }
        return item;
    }

    std::optional<GearOrSlot>
    To() const {
        if (slot > std::to_underlying(Gearslot::MAX)) {
            return std::nullopt;
        }
        if (unequip) {
            return static_cast<Gearslot>(slot);
        }
        auto* form = tes_util::GetForm(modname, id);
        if (!form) {
            return std::nullopt;
        }
        auto* extra_ench = (!extra_ench_modname.empty() || extra_ench_id)
                               ? tes_util::GetForm<RE::EnchantmentItem>(
                                   extra_ench_modname, extra_ench_id
                               )
                               : nullptr;
        return Gear::New(
            form, static_cast<Gearslot>(slot) == Gearslot::kLeft, extra_health, extra_ench
        );
    }
};

/// Similar to `Equipset`, ignored items == missing items.
class EquipsetSer final : public std::vector<EsItemSer> {
  public:
    static EquipsetSer
    From(const Equipset& equipset) {
        EquipsetSer v;
        std::transform(
            equipset.vec().cbegin(), equipset.vec().cend(), std::back_inserter(v), EsItemSer::From
        );
        return v;
    }

    Equipset
    To() const {
        std::vector<GearOrSlot> items;
        for (const auto& item_ser : *this) {
            auto item = item_ser.To();
            if (item) {
                items.push_back(*item);
            }
        }
        return Equipset(std::move(items));
    }
};

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
            auto* gos = QueryEquipset(equipset, item_ui.gos.slot());
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
        auto& v = equipset.vec();
        auto it = std::find_if(v.cbegin(), v.cend(), [=](const GearOrSlot& item) {
            return item.slot() == slot;
        });
        return it == v.cend() ? nullptr : &*it;
    }
};

template <typename K, typename E>
struct HotkeyIR final {
    std::string name = "Hotkey";
    std::vector<K> keysets;
    std::vector<E> equipsets;
    size_t active_equipset = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        HotkeyIR, name, keysets, equipsets, active_equipset
    );
};

template <typename K, typename E>
class HotkeysIR;
template <typename E>
HotkeysIR(const Hotkeys<E>&) -> HotkeysIR<Keyset, E>;
template <typename E>
HotkeysIR(const Hotkeys<E>&, bool) -> HotkeysIR<Keyset, E>;

template <typename K, typename E>
class HotkeysIR final {
  public:
    std::vector<HotkeyIR<K, E>> hotkeys;
    size_t active_hotkey;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(HotkeysIR, hotkeys, active_hotkey);

    explicit HotkeysIR(
        std::vector<HotkeyIR<K, E>> hks = {}, size_t active_hotkey = static_cast<size_t>(-1)
    )
        : hotkeys(std::move(hks)),
          active_hotkey(active_hotkey) {}

    /// If `reset_active` is true, discards the active state of hotkeys and equipsets (i.e. ensures
    /// no hotkey is active and for each hotkey, the first equipset is the active one).
    ///
    /// Persisting active state is only useful when serializing hotkey data to the SKSE co-save
    /// (i.e. persisting active status across save games).
    explicit HotkeysIR(const Hotkeys<E>& hks, bool reset_active = true)
        : active_hotkey(hks.active()) {
        std::transform(
            hks.vec().cbegin(),
            hks.vec().cend(),
            std::back_inserter(hotkeys),
            [](const Hotkey<E>& hotkey) {
                return HotkeyIR<Keyset, E>{
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
    Hotkeys<E>
    Into(bool reset_active = true)
    requires(std::is_same_v<K, Keyset>)
    {
        if (reset_active) {
            ResetActive();
        }
        std::vector<Hotkey<E>> hotkeys_out;
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out),
            [](HotkeyIR<Keyset, E>& hotkey_ir) {
                return Hotkey<E>{
                    .name = std::move(hotkey_ir.name),
                    .keysets = Keysets(std::move(hotkey_ir.keysets)),
                    .equipsets = Equipsets<E>(
                        std::move(hotkey_ir.equipsets), hotkey_ir.active_equipset
                    ),
                };
            }
        );
        return Hotkeys<E>(std::move(hotkeys_out), active_hotkey);
    }

    /// Cannibalizes HotkeysIR<K, E> to produce HotkeysIR<K1, E>.
    template <typename F>
    requires(std::is_invocable_v<F, const K&>)
    HotkeysIR<std::invoke_result_t<F, const K&>, E>
    ConvertKeyset(F f) {
        using NewK = std::invoke_result_t<F, const K&>;

        auto hotkeys_out = HotkeysIR<NewK, E>({}, active_hotkey);
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out.hotkeys),
            [&f](HotkeyIR<K, E>& hotkey) {
                auto hotkey_out = HotkeyIR<NewK, E>{
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

    /// Cannibalizes HotkeysIR<K, E> to produce HotkeysIR<K, E1>.
    template <typename F>
    requires(std::is_invocable_v<F, const E&>)
    HotkeysIR<K, std::invoke_result_t<F, const E&>>
    ConvertEquipset(F f) {
        using NewE = std::invoke_result_t<F, const E&>;

        auto hotkeys_out = HotkeysIR<K, NewE>({}, active_hotkey);
        std::transform(
            hotkeys.begin(),
            hotkeys.end(),
            std::back_inserter(hotkeys_out.hotkeys),
            [&f](HotkeyIR<K, E>& hotkey) {
                auto hotkey_out = HotkeyIR<K, NewE>{
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
        active_hotkey = static_cast<size_t>(-1);
        for (HotkeyIR<K, E>& hotkey : hotkeys) {
            hotkey.active_equipset = 0;
        }
    }
};

}  // namespace ech
