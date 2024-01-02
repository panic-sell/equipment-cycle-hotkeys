#pragma once

#include "tes_util.h"

namespace ech {

enum class Gearslot : uint8_t {
    /// 1h scrolls/spells/weapons, torches, shields.
    kLeft,
    /// 1h/2h scrolls/spells/weapons.
    kRight,
    /// Arrows or bolts.
    kAmmo,
    /// Shouts or other voice-equipped spells.
    kShout,
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

inline bool
ExtraHealthEq(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) {
        return true;
    }
    return std::fabsf(a - b) < .001f;
}

inline std::optional<Gearslot>
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
            "{} unequip failed: cannot look up {:08X} or {:08X}",
            left_hand ? Gearslot::kLeft : Gearslot::kRight,
            equipslot_id,
            tes_util::kWeapDummy
        );
    }
    //                                              queue, force, sounds, apply_now
    aem.EquipObject(&actor, dummy, nullptr, 1, slot, false, false, false, true);
    aem.UnequipObject(&actor, dummy, nullptr, 1, slot, false, false, false, true);
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
/// - A 2h scroll/spell/weapon will always be assigned `Gearslot::kRight`.
class Gear final {
  public:
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

    bool
    operator==(const Gear& other) const {
        return form_ == other.form_ && slot_ == other.slot_
               && internal::ExtraHealthEq(extra_health_, other.extra_health_)
               && extra_ench_ == other.extra_ench_;
    }

    /// Returns nullopt if `form` is not a supported gear type.
    ///
    /// `prefer_left` is ignored if `form` is not a 1h spell/weapon.
    static std::optional<Gear>
    New(RE::TESForm* form,
        bool prefer_left = false,
        float extra_health = std::numeric_limits<float>::quiet_NaN(),
        RE::EnchantmentItem* extra_ench = nullptr) {
        return internal::GetExpectedGearslot(form, prefer_left).transform([&](Gearslot slot) {
            return Gear(form, slot, extra_health, extra_ench);
        });
    }

    /// Returns nullopt if `slot` is empty or contains an unsupported gear type.
    ///
    /// This function does not check if the equipped item is in the player's inventory. Suppose the
    /// equipped item is a summoned bound sword; attempting to equip that later (equipping does
    /// check inventory) will fail because the player's inventory will only have the bound sword
    /// spell instead of the bound sword TESObjectWEAP.
    ///
    /// The reason this function doesn't check inventory is because there's no easy way to relate a
    /// summoned bound sword to the bound sword spell.
    static std::optional<Gear>
    FromEquipped(RE::Actor& actor, Gearslot slot) {
        std::optional<Gear> gear;
        switch (slot) {
            case Gearslot::kLeft:
                // Scroll handling must precede spell handling since scroll subclasses spell.
                gear = FromEquippedScroll(actor, true)
                           .or_else([&]() { return FromEquippedSpell(actor, true); })
                           .or_else([&]() { return FromEquippedWeapon(actor, true); })
                           .or_else([&]() { return FromEquippedTorch(actor); })
                           .or_else([&]() { return FromEquippedShield(actor); });
                break;
            case Gearslot::kRight:
                gear = FromEquippedScroll(actor, false)
                           .or_else([&]() { return FromEquippedSpell(actor, false); })
                           .or_else([&]() { return FromEquippedWeapon(actor, false); });
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

    /// When equipping 1h scrolls and weapons, there exists an edge case where if player swaps an
    /// item between hands, they will end up equipping in both hands even if only 1 item exists in
    /// inventory. This specific case is handled by unequipping the other hand first.
    void
    Equip(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto success = false;
        switch (slot_) {
            case Gearslot::kLeft:
                // Scroll handling must precede spell handling since scroll subclasses spell.
                // clang-format off
                success = EquipScroll(aem, actor)
                    || EquipSpell(aem, actor)
                    || EquipWeapon(aem, actor)
                    || EquipTorch(aem, actor)
                    || EquipShield(aem, actor);
                // clang-format on
                break;
            case Gearslot::kRight:
                // clang-format off
                success = EquipScroll(aem, actor)
                    || EquipSpell(aem, actor)
                    || EquipWeapon(aem, actor);
                // clang-format on
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
    FromEquippedScroll(const RE::Actor& actor, bool left_hand) {
        auto* form = actor.GetEquippedObject(left_hand);
        auto* scroll = form ? form->As<RE::ScrollItem>() : nullptr;
        if (!scroll || (left_hand && scroll->IsTwoHanded())) {
            return std::nullopt;
        }
        return New(scroll, left_hand);
    }

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

        auto extra_health = std::numeric_limits<float>::quiet_NaN();
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
        auto extra_health = std::numeric_limits<float>::quiet_NaN();
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
    EquipScroll(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto* scroll = form_->As<RE::ScrollItem>();
        if (!scroll) {
            return false;
        }
        auto invdata = FindMatchingInventoryData(actor);
        if (invdata.count <= 0) {
            return false;
        }
        const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(
            slot_ == Gearslot::kLeft ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand
        );
        if (!slot) {
            return false;
        }

        if (invdata.non_xl_count() <= 0 && !invdata.xls.unworn) {
            if (slot_ == Gearslot::kLeft && !invdata.xls.wornleft) {
                UnequipGear(aem, actor, Gearslot::kRight);
            } else if (slot_ == Gearslot::kRight && !invdata.xls.worn) {
                UnequipGear(aem, actor, Gearslot::kLeft);
            }
        }
        aem.EquipObject(&actor, scroll, nullptr, 1, slot, false, false, true, true);
        return true;
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
        auto invdata = FindMatchingInventoryData(actor);
        if (invdata.count <= 0) {
            return false;
        }
        const auto* slot = tes_util::GetForm<RE::BGSEquipSlot>(
            slot_ == Gearslot::kLeft ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand
        );
        if (!slot) {
            return false;
        }

        if (invdata.non_xl_count() <= 0 && !invdata.xls.unworn) {
            if (slot_ == Gearslot::kLeft && !invdata.xls.wornleft) {
                UnequipGear(aem, actor, Gearslot::kRight);
                invdata = FindMatchingInventoryData(actor);
            } else if (slot_ == Gearslot::kRight && !invdata.xls.worn) {
                UnequipGear(aem, actor, Gearslot::kLeft);
                invdata = FindMatchingInventoryData(actor);
            }
        }
        aem.EquipObject(
            &actor,
            form_->As<RE::TESBoundObject>(),
            invdata.xls.unworn,
            1,
            slot,
            false,
            false,
            true,
            true
        );
        return true;
    }

    [[nodiscard]] bool
    EquipTorch(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!form_->Is(RE::FormType::Light)) {
            return false;
        }
        if (FindMatchingInventoryData(actor).count <= 0) {
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
        auto invdata = FindMatchingInventoryData(actor);
        if (invdata.count <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor,
            form_->As<RE::TESBoundObject>(),
            invdata.xls.first(),
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
        auto invdata = FindMatchingInventoryData(actor);
        if (invdata.count <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor, form_->As<RE::TESBoundObject>(), nullptr, static_cast<uint32_t>(invdata.count)
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

    /// Extra lists in player inventory that match the current gear.
    struct MatchingExtraLists final {
        RE::ExtraDataList* unworn = nullptr;
        RE::ExtraDataList* worn = nullptr;
        RE::ExtraDataList* wornleft = nullptr;

        /// Returns the sum of each extra list's `GetCount()` result.
        int32_t
        count() const {
            return (unworn ? unworn->GetCount() : 0) + (worn ? worn->GetCount() : 0)
                   + (wornleft ? wornleft->GetCount() : 0);
        }

        /// Returns the first non-null extra list.
        RE::ExtraDataList*
        first() const {
            if (unworn) {
                return worn;
            }
            if (worn) {
                return worn;
            }
            return wornleft;
        }
    };

    struct MatchingInventoryData final {
        int32_t count = 0;
        MatchingExtraLists xls;

        int32_t
        non_xl_count() const {
            return count - xls.count();
        }
    };

    bool
    MatchesExtraList(const RE::ExtraDataList& xl) const {
        const auto* xl_extra_health = xl.GetByType<RE::ExtraHealth>();
        if (!internal::ExtraHealthEq(
                extra_health_,
                xl_extra_health ? xl_extra_health->health : std::numeric_limits<float>::quiet_NaN()
            )) {
            return false;
        }

        const auto* xl_extra_ench = xl.GetByType<RE::ExtraEnchantment>();
        if (extra_ench_ != (xl_extra_ench ? xl_extra_ench->enchantment : nullptr)) {
            return false;
        }

        return true;
    }

    /// Returns the first matching `RE::ExtraDataList` from `ied` along with the number of extra
    /// lists searched up to that point.
    MatchingExtraLists
    FindMatchingExtraLists(const RE::InventoryEntryData* ied) const {
        auto matches = MatchingExtraLists();
        tes_util::ForEachExtraList(ied, [&](RE::ExtraDataList& xl) {
            if (!MatchesExtraList(xl)) {
                return tes_util::ForEachExtraListControl::kContinue;
            }

            matches.unworn = &xl;
            if (xl.GetByType<RE::ExtraWorn>()) {
                matches.worn = &xl;
                matches.unworn = nullptr;
            }
            if (xl.GetByType<RE::ExtraWornLeft>()) {
                matches.wornleft = &xl;
                matches.unworn = nullptr;
            }

            return tes_util::ForEachExtraListControl::kContinue;
        });
        return matches;
    }

    /// Returns (1) count of matching inventory items and (2) the specific matching extra lists. #1
    /// can be greater than #2's `count()` if the gear has no extra data and matches inventory
    /// entries with no extra lists.
    ///
    /// A nonpositive count indicates "gear not found in inventory".
    ///
    /// This function is only meant for weapons, scrolls, shields, and ammo (i.e. not for spells or
    /// shouts).
    MatchingInventoryData
    FindMatchingInventoryData(RE::Actor& actor) const {
        auto inv = actor.GetInventory([&](const RE::TESBoundObject& obj) { return &obj == form_; });
        auto it = inv.cbegin();
        if (it == inv.cend()) {
            return {.count = 0};
        }
        const auto& [tot_count, ied] = it->second;
        if (tot_count <= 0) {
            return {.count = tot_count};
        }

        auto matching_xls = FindMatchingExtraLists(ied.get());
        auto matching_count = matching_xls.count();
        // Gear with no extra data should also match inventory entries with no extra lists.
        if (extra_ench_ == nullptr && std::isnan(extra_health_)) {
            matching_count += tot_count - tes_util::SumExtraListCounts(ied.get());
        }
        return {.count = matching_count, .xls = matching_xls};
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
    float extra_health_;
    RE::EnchantmentItem* extra_ench_;

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

    bool
    operator==(const GearOrSlot& other) const {
        const auto* a = gear();
        const auto* b = other.gear();
        if (a && b) {
            return *a == *b;
        }
        if (!a && !b) {
            return slot() == other.slot();
        }
        return false;
    }

  private:
    std::variant<ech::Gear, Gearslot> variant_;
};

}  // namespace ech
