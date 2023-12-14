#pragma once

#include "gear.h"
#include "hotkeys.h"
#include "keys.h"
#include "tes_util.h"

namespace ech {
namespace serde {

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

struct GearOrSlotSer final {
    uint8_t slot = 0;
    bool unequip = false;

    std::string modname = "";
    RE::FormID id = 0;
    float extra_health = std::numeric_limits<float>::quiet_NaN();
    std::string extra_ench_modname = "";
    RE::FormID extra_ench_id = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        GearOrSlotSer, slot, unequip, modname, id, extra_health, extra_ench_modname, extra_ench_id
    );

    static GearOrSlotSer
    From(const GearOrSlot& gos) {
        auto item = GearOrSlotSer{.slot = std::to_underlying(gos.slot())};
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

struct HotkeySer final {
    std::string name = "";
    std::vector<KeysetSer> keysets = {};
    std::vector<std::vector<GearOrSlotSer>> equipsets = {};
    size_t active_equipset = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        HotkeySer, name, keysets, equipsets, active_equipset
    );

    static HotkeySer
    From(const Hotkey<>& hotkey) {
        auto hotkey_ser = HotkeySer{
            .name = hotkey.name, .active_equipset = hotkey.equipsets.active()
        };
        std::transform(
            hotkey.keysets.vec().cbegin(),
            hotkey.keysets.vec().cend(),
            std::back_inserter(hotkey_ser.keysets),
            KeysetSer::From
        );
        std::transform(
            hotkey.equipsets.vec().cbegin(),
            hotkey.equipsets.vec().cend(),
            std::back_inserter(hotkey_ser.equipsets),
            [](const Equipset& equipset) {
                std::vector<GearOrSlotSer> v;
                std::transform(
                    equipset.vec().cbegin(),
                    equipset.vec().cend(),
                    std::back_inserter(v),
                    GearOrSlotSer::From
                );
                return v;
            }
        );
        return hotkey_ser;
    }

    Hotkey<>
    To() const {
        std::vector<Keyset> keysets_out;
        std::transform(
            keysets.cbegin(),
            keysets.cend(),
            std::back_inserter(keysets_out),
            [](const KeysetSer& keyset_ser) { return keyset_ser.To(); }
        );

        std::vector<Equipset> equipsets_out;
        std::transform(
            equipsets.cbegin(),
            equipsets.cend(),
            std::back_inserter(equipsets_out),
            [](const std::vector<GearOrSlotSer>& items_ser) {
                std::vector<GearOrSlot> items;
                for (const auto& item_ser : items_ser) {
                    auto item = item_ser.To();
                    if (item) {
                        items.push_back(std::move(*item));
                    }
                }
                return Equipset(std::move(items));
            }
        );

        return {
            .name = name,
            .keysets = Keysets(std::move(keysets_out)),
            .equipsets = Equipsets(std::move(equipsets_out), active_equipset),
        };
    }
};

struct HotkeysSer final {
    std::vector<HotkeySer> hotkeys = {};
    size_t active_hotkey = std::numeric_limits<size_t>::max();

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(HotkeysSer, hotkeys, active_hotkey);

    static HotkeysSer
    From(const Hotkeys<>& hotkeys) {
        auto hotkeys_ser = HotkeysSer{.active_hotkey = hotkeys.active()};
        std::transform(
            hotkeys.vec().cbegin(),
            hotkeys.vec().cend(),
            std::back_inserter(hotkeys_ser.hotkeys),
            HotkeySer::From
        );
        return hotkeys_ser;
    }

    Hotkeys<>
    To() const {
        std::vector<Hotkey<>> hotkeys_out;
        std::transform(
            hotkeys.cbegin(),
            hotkeys.cend(),
            std::back_inserter(hotkeys_out),
            [](const HotkeySer& hotkey_ser) { return hotkey_ser.To(); }
        );
        return Hotkeys<>(std::move(hotkeys_out), active_hotkey);
    }

    void
    ResetActive() {
        active_hotkey = hotkeys.size();
        for (auto& hotkey : hotkeys) {
            hotkey.active_equipset = 0;
        }
    }
};

inline nlohmann::json
SerializeKeyset(const Keyset& keyset) {
    nlohmann::json j;
    for (auto c : keyset) {
        if (KeycodeIsValid(c)) {
            j.push_back(c);
        }
    }
    return j;
}

inline std::optional<Keyset>
DeserializeKeyset(const nlohmann::json& j) {
    if (!j.is_array()) {
        return std::nullopt;
    }
    size_t sz = std::min(j.size(), std::tuple_size_v<Keyset>);
    Keyset keyset = {0};
    for (size_t i = 0; i < sz; i++) {
        auto name = j.at(i);
        if (!name.is_string()) {
            continue;
        }
        auto code = KeycodeFromName(name.template get<std::string>());
        if (!KeycodeIsValid(code)) {
            continue;
        }
        keyset[i] = code;
    }
    return keyset;
}

}  // namespace serde
}  // namespace ech
