#pragma once

#include "tes_util.h"

namespace ech {

enum class Gearslot : uint8_t {
    /// 1h spells, 1h weapons, torches, shields.
    kLeft,
    /// 1h/2h spells, 1h/2h weapons.
    kRight,
    /// Arrows or bolts.
    kAmmo,
    /// Shouts or other voice-equipped spells.
    kShout,

    MAX = kShout,
};

/// `kGearslots[i] == static_cast<Gearslot>(i)` for all `0 <= i < kGearslots.size()`
inline constexpr auto kGearslots = std::array{
    Gearslot::kLeft,
    Gearslot::kRight,
    Gearslot::kAmmo,
    Gearslot::kShout,
};

}  // namespace ech

template <>
struct fmt::formatter<ech::Gearslot> : fmt::formatter<std::string_view> {
    auto
    format(ech::Gearslot slot, format_context& ctx) const {
        auto name = "UNKNOWN SLOT"sv;
        switch (slot) {
            case ech::Gearslot::kLeft:
                name = "LEFT HAND";
                break;
            case ech::Gearslot::kRight:
                name = "RIGHT HAND";
                break;
            case ech::Gearslot::kAmmo:
                name = "AMMO SLOT";
                break;
            case ech::Gearslot::kShout:
                name = "VOICE SLOT";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

namespace ech {
namespace internal {

static std::optional<Gearslot>
GetExpectedGearslot(const RE::TESForm* form, bool prefer_left) {
    if (!form) {
        return std::nullopt;
    }

    switch (form->GetFormType()) {
        case RE::FormType::Ammo:
            return Gearslot::kAmmo;
        case RE::FormType::Shout:
            return Gearslot::kShout;
        case RE::FormType::Light:
            return Gearslot::kLeft;
        case RE::FormType::Armor:
            return tes_util::IsShield(form) ? std::optional(Gearslot::kLeft) : std::nullopt;
    }

    if (const auto* weap = form->As<RE::TESObjectWEAP>()) {
        return !prefer_left || tes_util::IsTwoHandedWeapon(weap) ? Gearslot::kRight
                                                                 : Gearslot::kLeft;
    }

    if (const auto* spell = form->As<RE::SpellItem>()) {
        if (tes_util::IsVoiceEquippable(spell)) {
            return Gearslot::kShout;
        }
        return !prefer_left || spell->IsTwoHanded() ? Gearslot::kRight : Gearslot::kLeft;
    }

    return std::nullopt;
}

inline void
UnequipHand(RE::ActorEquipManager& aem, RE::Actor& actor, bool left_hand) {
    auto equipslot_id = left_hand ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand;
    const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(equipslot_id);
    auto* dummy = tes_util::GetForm<RE::TESObjectWEAP>(tes_util::kWeapDummy);
    if (!slot || !dummy) {
        // Swallow the error and do nothing. Players can still unequip via menus.
        SKSE::log::error(
            "{} unequip failed: could not look up {:08X} or {:08X}",
            left_hand ? Gearslot::kLeft : Gearslot::kRight,
            equipslot_id,
            tes_util::kWeapDummy
        );
    }
    //                                              queue, force, sounds
    aem.EquipObject(&actor, dummy, nullptr, 1, slot, false, false, false);
    aem.UnequipObject(&actor, dummy, nullptr, 1, slot, false, false, false);
}

inline void
UnequipAmmo(RE::ActorEquipManager& aem, RE::Actor& actor) {
    auto* ammo = actor.GetCurrentAmmo();
    if (ammo) {
        aem.UnequipObject(&actor, ammo);
    }
}

inline void
UnequipShout(RE::Actor& actor) {
    auto* form = actor.GetActorRuntimeData().selectedPower;
    if (!form) {
        return;
    }
    if (auto* shout = form->As<RE::TESShout>()) {
        // Papyrus function Actor.UnequipShout
        using F = void(RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::Actor*, RE::TESShout*);
        auto f = REL::Relocation<F>{REL::RelocationID(53863, 54664)};
        f(nullptr, 0, &actor, shout);
        return;
    }
    if (auto* spell = form->As<RE::SpellItem>()) {
        // Papyrus function Actor.UnequipSpell
        using F = void(
            RE::BSScript::IVirtualMachine*, RE::VMStackID, RE::Actor*, RE::SpellItem*, int32_t
        );
        auto f = REL::Relocation<F>{REL::RelocationID(227784, 54669)};
        f(nullptr, 0, &actor, spell, 2);
        return;
    }
}

}  // namespace internal

inline void
UnequipGear(RE::ActorEquipManager& aem, RE::Actor& actor, Gearslot slot) {
    switch (slot) {
        case Gearslot::kLeft:
            internal::UnequipHand(aem, actor, true);
            break;
        case Gearslot::kRight:
            internal::UnequipHand(aem, actor, false);
            break;
        case Gearslot::kAmmo:
            internal::UnequipAmmo(aem, actor);
            break;
        case Gearslot::kShout:
            internal::UnequipShout(actor);
            break;
        default:
            SKSE::log::error("unknown slot {}", std::to_underlying(slot));
            return;
    }
    SKSE::log::trace("{} unequipped", slot);
}

/// Invariants:
/// - `form_` is non-null.
/// - `form_` is of a supported gear type per `GetExpectedGearslot()`.
/// - `extra_health == NaN` indicates weapon/shield has not been improved.
/// - `extra_ench == nullptr` indicates weapon/shield does not have custom enchantment.
class Gear final {
  public:
    static constexpr float kExtraHealthNone = std::numeric_limits<float>::quiet_NaN();

    const RE::TESForm&
    form() const {
        return *form_;
    }

    Gearslot
    slot() const {
        return slot_;
    }

    float
    extra_health() const {
        return extra_health_;
    }

    const RE::EnchantmentItem*
    extra_ench() const {
        return extra_ench_;
    }

    /// Returns nullopt if `form` is not a supported gear type.
    ///
    /// `prefer_left` is ignored if `form` is not a 1h spell/weapon.
    static std::optional<Gear>
    New(RE::TESForm* form,
        bool prefer_left = false,
        float extra_health = kExtraHealthNone,
        RE::EnchantmentItem* extra_ench = nullptr) {
        return internal::GetExpectedGearslot(form, prefer_left).transform([&](Gearslot slot) {
            return Gear(form, slot, extra_health, extra_ench);
        });
    }

    /// Returns nullopt if `slot` is empty or contains an unsupported gear type.
    static std::optional<Gear>
    FromEquipped(RE::Actor& actor, Gearslot slot) {
        std::optional<Gear> gear;
        switch (slot) {
            case Gearslot::kLeft:
                gear = FromEquippedSpell(actor, true)
                           .or_else([&]() { return FromEquippedWeapon(actor, true); })
                           .or_else([&]() { return FromEquippedTorch(actor); })
                           .or_else([&]() { return FromEquippedShield(actor); });
                break;
            case Gearslot::kRight:
                gear = FromEquippedSpell(actor, false).or_else([&]() {
                    return FromEquippedWeapon(actor, false);
                });
                break;
            case Gearslot::kAmmo:
                gear = New(actor.GetCurrentAmmo());
                break;
            case Gearslot::kShout:
                gear = New(actor.GetActorRuntimeData().selectedPower);
                break;
        }

        if (gear) {
            SKSE::log::trace("{} contains {}", slot, gear->form());
        } else {
            SKSE::log::trace("{} is empty or contains unsupported gear", slot);
        }
        return gear;
    }

    void
    Equip(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto success = false;
        switch (slot()) {
            case Gearslot::kLeft:
                success = EquipSpell(aem, actor) || EquipWeapon(aem, actor)
                          || EquipTorch(aem, actor) || EquipShield(aem, actor);
                break;
            case Gearslot::kRight:
                success = EquipSpell(aem, actor) || EquipWeapon(aem, actor);
                break;
            case Gearslot::kAmmo:
                success = EquipAmmo(aem, actor);
                break;
            case Gearslot::kShout:
                success = EquipShout(aem, actor);
                break;
        }

        if (success) {
            SKSE::log::trace("{} equipped {}", slot(), form());
        } else {
            SKSE::log::trace("{} ignored: {} not in inventory", slot(), form());
        }
    }

  private:
    static std::optional<Gear>
    FromEquippedSpell(const RE::Actor& actor, bool left_hand) {
        auto* form = actor.GetEquippedObject(left_hand);
        auto* spell = form ? form->As<RE::SpellItem>() : nullptr;
        if (!spell || (left_hand && spell->IsTwoHanded())) {
            return std::nullopt;
        }
        return New(spell, left_hand);
    }

    static std::optional<Gear>
    FromEquippedWeapon(const RE::Actor& actor, bool left_hand) {
        auto* weap = actor.GetEquippedObject(left_hand);
        const auto* ied = actor.GetEquippedEntryData(left_hand);
        // `ied` on an equipped weapon is guaranteed to have exactly 1 extra list.
        if (!weap || !weap->IsWeapon() || !ied || !ied->IsWorn()) {
            return std::nullopt;
        }
        // This check might be redundant because when looking at the left hand, a 2-handed weapon
        // will have `weap != nullptr` but `ied == nullptr`.
        if (left_hand && tes_util::IsTwoHandedWeapon(weap)) {
            return std::nullopt;
        }

        auto extra_health = kExtraHealthNone;
        RE::EnchantmentItem* extra_ench = nullptr;

        tes_util::ForEachExtraList(ied, [&](const RE::ExtraDataList& xl) {
            if (const auto* xhealth = xl.GetByType<RE::ExtraHealth>()) {
                extra_health = xhealth->health;
            }
            if (const auto* xench = xl.GetByType<RE::ExtraEnchantment>()) {
                extra_ench = xench->enchantment;
            }
            return tes_util::ForEachExtraListControl::kBreak;
        });

        return New(weap, left_hand, extra_health, extra_ench);
    }

    static std::optional<Gear>
    FromEquippedTorch(const RE::Actor& actor) {
        auto* torch = actor.GetEquippedObject(true);
        if (!torch || !torch->Is(RE::FormType::Light)) {
            return std::nullopt;
        }
        return New(torch);
    }

    /// `actor` is not a const because `actor.GetInventory()` may mutate it.
    static std::optional<Gear>
    FromEquippedShield(RE::Actor& actor) {
        auto inv = actor.GetInventory([](const RE::TESBoundObject& obj) {
            return tes_util::IsShield(&obj);
        });
        auto it = inv.cbegin();
        if (it == inv.cend()) {
            return std::nullopt;
        }
        auto* shield = it->first;
        auto count = it->second.first;
        const auto* ied = it->second.second.get();
        if (!shield || count <= 0) {
            return std::nullopt;
        }

        auto equipped = false;
        auto extra_health = kExtraHealthNone;
        RE::EnchantmentItem* extra_ench = nullptr;

        tes_util::ForEachExtraList(ied, [&](const RE::ExtraDataList& xl) {
            if (!xl.HasType<RE::ExtraWorn>() && !xl.HasType<RE::ExtraWornLeft>()) {
                return tes_util::ForEachExtraListControl::kContinue;
            }
            equipped = true;
            if (const auto* xhealth = xl.GetByType<RE::ExtraHealth>()) {
                extra_health = xhealth->health;
            }
            if (const auto* xench = xl.GetByType<RE::ExtraEnchantment>()) {
                extra_ench = xench->enchantment;
            }
            return tes_util::ForEachExtraListControl::kBreak;
        });

        if (!equipped) {
            return std::nullopt;
        }
        return New(shield, false, extra_health, extra_ench);
    }

    [[nodiscard]] bool
    EquipSpell(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto* spell = form_->As<RE::SpellItem>();
        if (!spell || !actor.HasSpell(spell)) {
            return false;
        }
        const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(
            slot_ == Gearslot::kLeft ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand
        );
        if (!slot) {
            return false;
        }
        aem.EquipSpell(&actor, spell, slot);
        return true;
    }

    [[nodiscard]] bool
    EquipWeapon(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!form_->IsWeapon()) {
            return false;
        }
        const auto& [count, xl] = FindInventoryData(actor);
        if (count <= 0) {
            return false;
        }
        const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(
            slot_ == Gearslot::kLeft ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand
        );
        if (!slot) {
            return false;
        }
        aem.EquipObject(&actor, form_->As<RE::TESBoundObject>(), xl, 1, slot);
        return true;
    }

    [[nodiscard]] bool
    EquipTorch(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!form_->Is(RE::FormType::Light)) {
            return false;
        }
        auto count = FindInventoryData(actor).first;
        if (count <= 0) {
            return false;
        }
        aem.EquipObject(&actor, form_->As<RE::TESBoundObject>());
        return true;
    }

    [[nodiscard]] bool
    EquipShield(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!tes_util::IsShield(form_)) {
            return false;
        }
        const auto& [count, xl] = FindInventoryData(actor);
        if (count <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor,
            form_->As<RE::TESBoundObject>(),
            xl,
            1,
            form_->As<RE::TESObjectARMO>()->GetEquipSlot()
        );
        return true;
    }

    [[nodiscard]] bool
    EquipAmmo(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!form_->IsAmmo()) {
            return false;
        }
        auto count = FindInventoryData(actor).first;
        if (count <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor, form_->As<RE::TESBoundObject>(), nullptr, static_cast<uint32_t>(count)
        );
        return true;
    }

    [[nodiscard]] bool
    EquipShout(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (auto* shout = form_->As<RE::TESShout>()) {
            if (!actor.HasShout(shout)) {
                return false;
            }
            aem.EquipShout(&actor, shout);
            return true;
        }
        if (auto* spell = form_->As<RE::SpellItem>()) {
            if (!actor.HasSpell(spell)) {
                return false;
            }
            const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(tes_util::kEqupVoice);
            if (!slot) {
                return false;
            }
            aem.EquipSpell(&actor, spell, slot);
            return true;
        }
        return false;
    }

    bool
    MatchesExtraList(const RE::ExtraDataList& xl) const {
        const auto* a = xl.GetByType<RE::ExtraEnchantment>();
        auto* extra_ench = a ? a->enchantment : nullptr;
        if (extra_ench != extra_ench_) {
            return false;
        }

        const auto* b = xl.GetByType<RE::ExtraHealth>();
        auto extra_health = b ? b->health : std::numeric_limits<float>::quiet_NaN();
        if (std::isnan(extra_health) && std::isnan(extra_health_)) {
            return true;
        }
        return std::fabsf(extra_health - extra_health_) < .001f;
    }

    /// Returns the first matching `RE::ExtraDataList` from `ied` along with the number of extra
    /// lists searched up to that point.
    RE::ExtraDataList*
    FindMatchingExtraList(const RE::InventoryEntryData* ied) const {
        RE::ExtraDataList* outer_xl = nullptr;
        tes_util::ForEachExtraList(ied, [&](RE::ExtraDataList& xl) {
            if (MatchesExtraList(xl)) {
                outer_xl = &xl;
                return tes_util::ForEachExtraListControl::kBreak;
            }
            return tes_util::ForEachExtraListControl::kContinue;
        });
        return outer_xl;
    }

    /// Returns (1) count of matching inventory items and (2) the specific matching extra list.
    ///
    /// A nonpositive count indicates "gear not found in inventory".
    ///
    /// This function is only meant for weapons, shields, and ammo (i.e. not for spells or shouts).
    std::pair<int32_t, RE::ExtraDataList*>
    FindInventoryData(RE::Actor& actor) const {
        auto inv = actor.GetInventory([&](const RE::TESBoundObject& obj) { return &obj == form_; });
        auto it = inv.cbegin();
        if (it == inv.cend()) {
            return {0, nullptr};
        }
        const auto& [tot_count, ied] = it->second;
        if (tot_count <= 0) {
            return {tot_count, nullptr};
        }

        auto* xl = FindMatchingExtraList(ied.get());
        if (xl) {
            return {xl->GetCount(), xl};
        }

        // At this point, gear doesn't match any inventory extra lists, so the only possible match
        // is between {gear with no extra data} and {inventory entry with no extra list}.
        if (extra_ench_ != nullptr || !std::isnan(extra_health_)) {
            return {0, nullptr};
        }
        auto xl_count = tes_util::GetExtraListCount(ied.get());
        return {tot_count - xl_count, nullptr};
    }

    explicit Gear(
        RE::TESForm* form, Gearslot slot, float extra_health, RE::EnchantmentItem* extra_ench
    )
        : form_(form),
          slot_(slot),
          extra_health_(extra_health),
          extra_ench_(extra_ench) {}

    RE::TESForm* form_;
    Gearslot slot_;
    float extra_health_ = kExtraHealthNone;
    RE::EnchantmentItem* extra_ench_ = nullptr;

    friend Gear GearForTest(Gearslot);
};

/// Like a `std::variant<Gear, Gearslot>`.
class GearOrSlot final {
  public:
    GearOrSlot(ech::Gear gear) : variant_(gear) {}

    GearOrSlot(Gearslot slot) : variant_(slot) {}

    /// Returns nullptr if this object is storing a `Gearslot`.
    const ech::Gear*
    gear() const {
        return std::get_if<ech::Gear>(&variant_);
    }

    Gearslot
    slot() const {
        if (gear()) {
            return gear()->slot();
        }
        if (const auto* slot = std::get_if<Gearslot>(&variant_)) {
            return *slot;
        }
        return static_cast<Gearslot>(0);
    }

  private:
    std::variant<ech::Gear, Gearslot> variant_;
};

}  // namespace ech
