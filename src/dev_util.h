// Development utilities.
#pragma once

#include "equipsets.h"
#include "gear.h"
#include "hotkeys.h"
#include "keys.h"
#include "tes_util.h"

namespace ech {
namespace dev_util {

using StrKeyset = std::array<std::string_view, std::tuple_size<Keyset>::value>;

inline Keysets
KeysetsFromStr(std::initializer_list<StrKeyset> keysets) {
    std::vector<Keyset> v;
    for (auto strkeyset : keysets) {
        Keyset keyset;
        for (size_t i = 0; i < keyset.size(); i++) {
            keyset[i] = KeycodeFromName(strkeyset[i]);
        }
        v.push_back(std::move(keyset));
    }
    return Keysets(std::move(v));
}

inline std::string
KeystrokesToStr(std::span<const Keystroke> keystrokes) {
    std::string s;
    size_t i = 0;
    for (const auto& ks : keystrokes) {
        s.append("{");
        s.append(KeycodeName(ks.Keycode()));
        s.append(": ");
        s.append(std::format("{:.2f}", ks.Heldsecs()));
        s.append("}");
        i++;
        if (i < keystrokes.size()) {
            s.append(" ");
        }
    }
    return s;
}

struct InputAction {
    Keysets keysets;
    std::function<void()> invoke;
    Keysets::MatchResult match_res = Keysets::MatchResult::kPress;
};

const InputAction*
GetFirstMatchingAction(
    std::span<const InputAction> actions, std::span<const Keystroke> keystrokes
) {
    for (const auto& ia : actions) {
        if (ia.keysets.Match(keystrokes) == ia.match_res) {
            return &ia;
        }
    }
    return nullptr;
}

namespace input_handlers {

void
InspectEquipped(std::span<const Keystroke> keystrokes, RE::Actor& actor) {
    std::optional<Gear> gear;
    RE::TESForm* obj = nullptr;
    RE::InventoryEntryData* ied = nullptr;

    RE::TESObjectWEAP* obj_weap = nullptr;
    RE::TESObjectARMO* obj_shield = nullptr;
    RE::SpellItem* obj_spell = nullptr;
    RE::TESAmmo* obj_ammo = nullptr;
    RE::TESShout* obj_shout = nullptr;

    auto f = [&](Gearslot slot) {
        gear = Gear::FromEquipped(actor, slot);
        switch (slot) {
            case Gearslot::kLeft:
                obj = actor.GetEquippedObject(true);
                ied = actor.GetEquippedEntryData(true);
                break;
            case Gearslot::kRight:
                obj = actor.GetEquippedObject(false);
                ied = actor.GetEquippedEntryData(false);
                break;
            case Gearslot::kAmmo:
                obj = actor.GetCurrentAmmo();
                break;
            case Gearslot::kShout:
                obj = actor.GetActorRuntimeData().selectedPower;
                break;
        }
    };

    auto actions = std::vector<InputAction>{
        {.keysets = KeysetsFromStr({{"1"}}), .invoke = std::bind(f, Gearslot::kLeft)},
        {.keysets = KeysetsFromStr({{"2"}}), .invoke = std::bind(f, Gearslot::kRight)},
        {.keysets = KeysetsFromStr({{"3"}}), .invoke = std::bind(f, Gearslot::kAmmo)},
        {.keysets = KeysetsFromStr({{"4"}}), .invoke = std::bind(f, Gearslot::kShout)},
    };
    auto* action = GetFirstMatchingAction(actions, keystrokes);
    if (!action) {
        return;
    }
    action->invoke();

    if (obj) {
        obj_weap = obj->As<RE::TESObjectWEAP>();
        obj_shield = obj->As<RE::TESObjectARMO>();
        obj_spell = obj->As<RE::SpellItem>();
        obj_ammo = obj->As<RE::TESAmmo>();
        obj_shout = obj->As<RE::TESShout>();
    }
}

const Equipset*
UseHotkeys(Hotkeys<>& hotkeys, std::span<const Keystroke> keystrokes, RE::Actor& actor) {
    auto set = [&](size_t i) {
        auto hotkeys_ir = HotkeysIr<Equipset>::From(hotkeys);
        auto equipset = Equipset::FromEquipped(actor, true);
        hotkeys_ir[i].equipsets.push_back(std::move(equipset));
        hotkeys = hotkeys_ir.To();
        SKSE::log::info("added equipset to hotkey {}", i + 1);
    };
    auto remove = [&](size_t i) {
        auto hotkeys_ir = HotkeysIr<Equipset>::From(hotkeys);
        auto& equipsets = hotkeys_ir[i].equipsets;
        if (equipsets.empty()) {
            return;
        }
        equipsets.pop_back();
        hotkeys = hotkeys_ir.To();
        SKSE::log::info("removed equipset from hotkey {}", i + 1);
    };

    const Equipset* next_equipset = nullptr;
    auto equip = [&]() { next_equipset = hotkeys.GetNextEquipset(keystrokes); };

    auto actions = std::vector<InputAction>{
        {.keysets = KeysetsFromStr({{"lshift", "1"}}), .invoke = std::bind(set, 0)},
        {.keysets = KeysetsFromStr({{"lshift", "2"}}), .invoke = std::bind(set, 1)},
        {.keysets = KeysetsFromStr({{"lshift", "3"}}), .invoke = std::bind(set, 2)},
        {.keysets = KeysetsFromStr({{"lshift", "4"}}), .invoke = std::bind(set, 3)},
        {.keysets = KeysetsFromStr({{"rshift", "1"}}), .invoke = std::bind(remove, 0)},
        {.keysets = KeysetsFromStr({{"rshift", "2"}}), .invoke = std::bind(remove, 1)},
        {.keysets = KeysetsFromStr({{"rshift", "3"}}), .invoke = std::bind(remove, 2)},
        {.keysets = KeysetsFromStr({{"rshift", "4"}}), .invoke = std::bind(remove, 3)},
        {.keysets = KeysetsFromStr({{"1"}, {"2"}, {"3"}, {"4"}}), .invoke = equip},
        {
            .keysets = KeysetsFromStr({{"1"}, {"2"}, {"3"}, {"4"}}),
            .invoke = equip,
            .match_res = Keysets::MatchResult::kSemihold,
        },
        {
            .keysets = KeysetsFromStr({{"1"}, {"2"}, {"3"}, {"4"}}),
            .invoke = equip,
            .match_res = Keysets::MatchResult::kHold,
        },
        {
            .keysets = KeysetsFromStr({{"5"}}),
            .invoke = []() { SKSE::log::info("{}", RE::GetDurationOfApplicationRunTime()); },
        },
    };
    auto* action = GetFirstMatchingAction(actions, keystrokes);
    if (!action) {
        return nullptr;
    }
    action->invoke();
    return next_equipset;
}

void
EquipSingleItem(
    std::span<const Keystroke> keystrokes, RE::ActorEquipManager& aem, RE::Actor& actor
) {
    static std::optional<Gear> equips[4];
    auto unequip = [&](size_t i) { UnequipGear(aem, actor, static_cast<Gearslot>(i)); };
    auto equip = [&](size_t i) {
        auto& gear = equips[i];
        if (!gear) {
            SKSE::log::trace("no gear in slot {}", i);
            return;
        }
        gear->Equip(aem, actor);
    };
    auto set = [&](size_t i) {
        auto eq = Gear::FromEquipped(actor, static_cast<Gearslot>(i));
        if (eq) {
            equips[i] = eq;
        }
    };

    auto actions = std::vector<InputAction>{
        {.keysets = KeysetsFromStr({{"lshift", "1"}}), .invoke = std::bind(unequip, 0)},
        {.keysets = KeysetsFromStr({{"lshift", "2"}}), .invoke = std::bind(unequip, 1)},
        {.keysets = KeysetsFromStr({{"lshift", "3"}}), .invoke = std::bind(unequip, 2)},
        {.keysets = KeysetsFromStr({{"lshift", "4"}}), .invoke = std::bind(unequip, 3)},
        {.keysets = KeysetsFromStr({{"rshift", "1"}}), .invoke = std::bind(set, 0)},
        {.keysets = KeysetsFromStr({{"rshift", "2"}}), .invoke = std::bind(set, 1)},
        {.keysets = KeysetsFromStr({{"rshift", "3"}}), .invoke = std::bind(set, 2)},
        {.keysets = KeysetsFromStr({{"rshift", "4"}}), .invoke = std::bind(set, 3)},
        {.keysets = KeysetsFromStr({{"1"}}), .invoke = std::bind(equip, 0)},
        {.keysets = KeysetsFromStr({{"2"}}), .invoke = std::bind(equip, 1)},
        {.keysets = KeysetsFromStr({{"3"}}), .invoke = std::bind(equip, 2)},
        {.keysets = KeysetsFromStr({{"4"}}), .invoke = std::bind(equip, 3)},
    };
    auto* action = GetFirstMatchingAction(actions, keystrokes);
    if (!action) {
        return;
    }
    action->invoke();
}

void
EquipMultipleItems(
    std::span<const Keystroke> keystrokes, RE::ActorEquipManager& aem, RE::Actor& actor
) {
    static std::array<Equipset, 4> equipsets;
    auto actions = std::vector<InputAction>{
        {
            .keysets = KeysetsFromStr({{"lshift", "1"}}),
            .invoke = [&]() { equipsets[0] = Equipset::FromEquipped(actor); },
        },
        {
            .keysets = KeysetsFromStr({{"lshift", "2"}}),
            .invoke = [&]() { equipsets[1] = Equipset::FromEquipped(actor); },
        },
        {
            .keysets = KeysetsFromStr({{"lshift", "3"}}),
            .invoke = [&]() { equipsets[2] = Equipset::FromEquipped(actor); },
        },
        {
            .keysets = KeysetsFromStr({{"lshift", "4"}}),
            .invoke = [&]() { equipsets[3] = Equipset::FromEquipped(actor); },
        },
        {
            .keysets = KeysetsFromStr({{"rshift", "1"}}),
            .invoke = [&]() { equipsets[0] = Equipset::FromEquipped(actor, true); },
        },
        {
            .keysets = KeysetsFromStr({{"rshift", "2"}}),
            .invoke = [&]() { equipsets[1] = Equipset::FromEquipped(actor, true); },
        },
        {
            .keysets = KeysetsFromStr({{"rshift", "3"}}),
            .invoke = [&]() { equipsets[2] = Equipset::FromEquipped(actor, true); },
        },
        {
            .keysets = KeysetsFromStr({{"rshift", "4"}}),
            .invoke = [&]() { equipsets[3] = Equipset::FromEquipped(actor, true); },
        },
        {
            .keysets = KeysetsFromStr({{"1"}}),
            .invoke = [&]() { equipsets[0].Apply(aem, actor); },
        },
        {
            .keysets = KeysetsFromStr({{"2"}}),
            .invoke = [&]() { equipsets[1].Apply(aem, actor); },
        },
        {
            .keysets = KeysetsFromStr({{"3"}}),
            .invoke = [&]() { equipsets[2].Apply(aem, actor); },
        },
        {
            .keysets = KeysetsFromStr({{"4"}}),
            .invoke = [&]() { equipsets[3].Apply(aem, actor); },
        },
    };
    auto* action = GetFirstMatchingAction(actions, keystrokes);
    if (!action) {
        return;
    }
    action->invoke();
}

void
EquipNonexistentItem(
    std::span<const Keystroke> keystrokes, RE::ActorEquipManager& aem, RE::Actor& actor
) {
    RE::TESForm* form = nullptr;
    std::optional<Gear> gear;
    auto actions = std::vector<InputAction>{
        // Staff of Banishing
        {.keysets = KeysetsFromStr({{"1"}}), .invoke = [&]() { form = tes_util::GetForm(0x29b79); }
        },
        // Volendrung
        {.keysets = KeysetsFromStr({{"2"}}), .invoke = [&]() { form = tes_util::GetForm(0x2acd2); }
        },
        // Daedric Sword
        {.keysets = KeysetsFromStr({{"3"}}), .invoke = [&]() { form = tes_util::GetForm(0x139B9); }
        },
        // Mass Paralysis
        {.keysets = KeysetsFromStr({{"4"}}), .invoke = [&]() { form = tes_util::GetForm(0xB62E6); }
        },
        // Fireball
        {.keysets = KeysetsFromStr({{"5"}}), .invoke = [&]() { form = tes_util::GetForm(0x1C789); }
        },
        // Glass Arrow
        {.keysets = KeysetsFromStr({{"6"}}), .invoke = [&]() { form = tes_util::GetForm(0x139BE); }
        },
    };
    auto* action = GetFirstMatchingAction(actions, keystrokes);
    if (!action) {
        return;
    }
    action->invoke();

    gear = Gear::New(form, true);
    if (gear) {
        gear->Equip(aem, actor);
    }
}

}  // namespace input_handlers
}  // namespace dev_util
}  // namespace ech
