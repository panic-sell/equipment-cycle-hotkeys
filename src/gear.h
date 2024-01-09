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
        case RE::FormType::Weapon:
            return !prefer_left || tes_util::IsTwoHandedWeapon(form) ? Gearslot::kRight
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
    auto equp_id = left_hand ? tes_util::kEqupLeftHand : tes_util::kEqupRightHand;
    const auto* bgs_slot = tes_util::GetForm<RE::BGSEquipSlot>(equp_id);
    auto* dummy = tes_util::GetForm<RE::TESObjectWEAP>(tes_util::kWeapDummy);
    if (!bgs_slot || !dummy) {
        SKSE::log::error(
            "{} unequip failed: cannot look up {:08X} or {:08X}",
            left_hand ? Gearslot::kLeft : Gearslot::kRight,
            equp_id,
            tes_util::kWeapDummy
        );
        // Swallow the error and do nothing. Players can still unequip via menus.
        return;
    }
    //                                                 queue, force, sounds, apply_now
    aem.EquipObject(&actor, dummy, nullptr, 1, bgs_slot, false, false, false, true);
    aem.UnequipObject(&actor, dummy, nullptr, 1, bgs_slot, false, false, false, true);
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
/// - `extra_.health == NaN` indicates weapon/shield has not been improved.
/// - `extra_.ench == nullptr` indicates weapon/shield does not have custom enchantment.
/// - A 2h scroll/spell/weapon will always be assigned `Gearslot::kRight`.
class Gear final {
  public:
    struct Extra final {
        /// This is specifically extra text with `DisplayDataType::kCustomName`. I.e. base names
        /// modified by extra health will not be saved here.
        std::string name;

        /// Most likely points to a 0xFF* custom enchantment.
        RE::EnchantmentItem* ench = nullptr;

        bool
        operator==(const Extra& other) const {
            return name == other.name && ench == other.ench;
        }

        // Empty name is considered equivalent to any name. This enables a hotkeyed gear with no
        // custom name to match inventory items with custom names, but a hotkeyed gear with custom
        // name will only match inventory items with the same custom name.
        bool
        EquivalentTo(const Extra& other) const {
            if (ench != other.ench) {
                return false;
            }
            if (!name.empty() && name != other.name) {
                return false;
            }
            return true;
        }

        static Extra
        FromXL(RE::ExtraDataList* xl) {
            if (!xl) {
                return {};
            }
            auto extra = Extra();
            if (const auto* xtext = xl->GetByType<RE::ExtraTextDisplayData>()) {
                if (xtext->IsPlayerSet()) {
                    extra.name = xtext->displayName;
                }
            }
            if (const auto* xench = xl->GetByType<RE::ExtraEnchantment>()) {
                extra.ench = xench->enchantment;
            }
            return extra;
        }
    };

    const RE::TESForm&
    form() const {
        return *form_;
    }

    Gearslot
    slot() const {
        return slot_;
    }

    const char*
    name() const {
        if (!extra().name.empty()) {
            return extra().name.c_str();
        }
        if (*form().GetName()) {
            return form().GetName();
        }
        return "<MISSING NAME>";
    }

    const Extra&
    extra() const {
        return extra_;
    }

    bool
    operator==(const Gear& other) const {
        return form_ == other.form_ && slot() == other.slot() && extra() == other.extra();
    }

    /// Returns nullopt if `form` is null or not a supported gear type.
    ///
    /// `prefer_left` is ignored if `form` is not a 1h scroll/spell/weapon.
    static std::optional<Gear>
    New(RE::TESForm* form, bool prefer_left = false, Extra extra = {}) {
        return internal::GetExpectedGearslot(form, prefer_left).transform([&](Gearslot slot) {
            return Gear(form, slot, extra);
        });
    }

#ifdef ECH_TEST
    static Gear
    NewForTest(Gearslot slot) {
        return Gear(nullptr, slot, {});
    }
#endif

    /// Returns nullopt if `slot` is empty.
    ///
    /// This function does not check if the equipped item is in the player's inventory. Suppose
    /// the equipped item is a summoned bound sword; attempting to equip that later (equipping
    /// does check inventory) will fail because the player's inventory will only have the bound
    /// sword spell instead of the bound sword TESObjectWEAP.
    ///
    /// The reason this function doesn't check inventory is because there's no easy way to
    /// relate a summoned bound sword to the bound sword spell.
    static std::optional<Gear>
    FromEquipped(RE::Actor& actor, Gearslot slot) {
        auto out = std::optional<Gear>();
        switch (slot) {
            case Gearslot::kLeft:
                // Scroll handling must precede spell handling since scroll subclasses spell.
                out = FromEquippedScroll(actor, true)
                          .or_else([&]() { return FromEquippedSpell(actor, true); })
                          .or_else([&]() { return FromEquippedWeapon(actor, true); })
                          .or_else([&]() { return FromEquippedTorch(actor); })
                          .or_else([&]() { return FromEquippedShield(actor); });
                break;
            case Gearslot::kRight:
                out = FromEquippedScroll(actor, false)
                          .or_else([&]() { return FromEquippedSpell(actor, false); })
                          .or_else([&]() { return FromEquippedWeapon(actor, false); });
                break;
            case Gearslot::kAmmo:
                out = New(actor.GetCurrentAmmo());
                break;
            case Gearslot::kShout:
                out = New(actor.GetActorRuntimeData().selectedPower);
                break;
        }

        if (out) {
            SKSE::log::trace("{} contains {}", slot, out->form());
        } else {
            SKSE::log::trace("{} is empty", slot);
        }

        return out;
    }

    /// When equipping 1h scrolls and weapons, there exists an edge case where if player swaps an
    /// item between hands, they will end up equipping it in both hands even if there is only one
    /// item in the player's inventory. This specific case is handled by unequipping the other hand
    /// first.
    void
    Equip(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto success = false;
        switch (slot()) {
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
        // This check might be redundant because when looking at the left hand, a 2-handed
        // weapon will have `weap != nullptr` but `ied == nullptr`.
        if (left_hand && tes_util::IsTwoHandedWeapon(weap)) {
            return std::nullopt;
        }

        for (auto* xl : tes_util::GetXLs(ied)) {
            return New(weap, left_hand, Extra::FromXL(xl));
        }
        return std::nullopt;
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
        RE::TESBoundObject* shield = it->first;
        RE::TESObjectREFR::Count count = it->second.first;
        const auto* ied = it->second.second.get();
        if (!shield || count <= 0) {
            return std::nullopt;
        }

        for (auto* xl : tes_util::GetXLs(ied)) {
            if (xl->HasType<RE::ExtraWorn>() || xl->HasType<RE::ExtraWornLeft>()) {
                return New(shield, true, Extra::FromXL(xl));
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool
    EquipScroll(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto* scroll = form_->As<RE::ScrollItem>();
        if (!scroll) {
            return false;
        }
        const auto& [count_tot, xls] = GetMatchingInvData(actor);
        if (count_tot <= 0) {
            return false;
        }
        if (count_tot == 1) {
            if (slot() == Gearslot::kLeft && GetFirstMatchingXL(xls, XLWornType::kWorn)) {
                UnequipGear(aem, actor, Gearslot::kRight);
            } else if (slot() == Gearslot::kRight && GetFirstMatchingXL(xls, XLWornType::kWornLeft)) {
                UnequipGear(aem, actor, Gearslot::kLeft);
            }
        }
        aem.EquipObject(&actor, scroll, nullptr, 1, GetBGSEquipSlot(), false, false, true, true);
        return true;
    }

    [[nodiscard]] bool
    EquipSpell(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        auto* spell = form_->As<RE::SpellItem>();
        if (!spell || !actor.HasSpell(spell)) {
            return false;
        }
        aem.EquipSpell(&actor, spell, GetBGSEquipSlot());
        return true;
    }

    [[nodiscard]] bool
    EquipWeapon(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        if (!form_->IsWeapon()) {
            return false;
        }
        auto invdata = GetMatchingInvData(actor);
        if (invdata.first <= 0) {
            return false;
        }

        if (invdata.first == 1) {
            if (slot() == Gearslot::kLeft
                && GetFirstMatchingXL(invdata.second, XLWornType::kWorn)) {
                UnequipGear(aem, actor, Gearslot::kRight);
                invdata = GetMatchingInvData(actor);
            } else if (slot() == Gearslot::kRight && GetFirstMatchingXL(invdata.second, XLWornType::kWornLeft)) {
                UnequipGear(aem, actor, Gearslot::kLeft);
                invdata = GetMatchingInvData(actor);
            }
        }
        const auto& [count_tot, xls] = invdata;

        RE::ExtraDataList* xl = nullptr;
        if (count_tot - tes_util::SumXLCounts(xls) > 0) {
            // Matches an inventory item with no extra list.
        } else if (slot() == Gearslot::kLeft) {
            xl = GetFirstMatchingXL(xls, std::array{XLWornType::kWornLeft, XLWornType::kUnworn});
        } else if (slot() == Gearslot::kRight) {
            xl = GetFirstMatchingXL(xls, std::array{XLWornType::kWorn, XLWornType::kUnworn});
        }
        aem.EquipObject(
            &actor,
            form_->As<RE::TESBoundObject>(),
            xl,
            1,
            GetBGSEquipSlot(),
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
        const auto& [count_tot, _] = GetMatchingInvData(actor);
        if (count_tot <= 0) {
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
        const auto& [count_tot, xls] = GetMatchingInvData(actor);
        if (count_tot <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor,
            form_->As<RE::TESBoundObject>(),
            GetFirstMatchingXL(xls, XLWornType::kAny),
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
        const auto& [count_tot, _] = GetMatchingInvData(actor);
        if (count_tot <= 0) {
            return false;
        }
        aem.EquipObject(
            &actor, form_->As<RE::TESBoundObject>(), nullptr, static_cast<uint32_t>(count_tot)
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
            aem.EquipSpell(&actor, spell, GetBGSEquipSlot());
            return true;
        }
        return false;
    }

    enum class XLWornType {
        kAny,
        kUnworn,
        kWorn,
        kWornLeft,
    };

    bool
    MatchesXL(RE::ExtraDataList& xl, XLWornType t) const {
        if (!extra().EquivalentTo(Extra::FromXL(&xl))) {
            return false;
        }
        switch (t) {
            case XLWornType::kAny:
                return true;
            case XLWornType::kUnworn:
                return !xl.GetByType<RE::ExtraWorn>() && !xl.GetByType<RE::ExtraWornLeft>();
            case XLWornType::kWorn:
                return xl.GetByType<RE::ExtraWorn>();
            case XLWornType::kWornLeft:
                return xl.GetByType<RE::ExtraWornLeft>();
        }
        return false;
    }

    RE::ExtraDataList*
    GetFirstMatchingXL(std::span<RE::ExtraDataList* const> xls, XLWornType t) const {
        auto it = std::find_if(xls.begin(), xls.end(), [&](RE::ExtraDataList* xl) {
            return xl && MatchesXL(*xl, t);
        });
        return it == xls.end() ? nullptr : *it;
    }

    RE::ExtraDataList*
    GetFirstMatchingXL(std::span<RE::ExtraDataList* const> xls, std::span<const XLWornType> types)
        const {
        for (auto t : types) {
            if (auto* xl = GetFirstMatchingXL(xls, t)) {
                return xl;
            }
        }
        return nullptr;
    }

    /// Returns (1) count of matching inventory items and (2) the specific matching extra lists.
    /// #1 can be greater than #2's total count if the gear has no extra data and matches
    /// inventory entries with no extra lists.
    ///
    /// A nonpositive count indicates "gear not found in inventory".
    ///
    /// This function is only meant for weapons, scrolls, shields, and ammo (i.e. not for spells
    /// or shouts).
    std::pair<int32_t, std::vector<RE::ExtraDataList*>>
    GetMatchingInvData(RE::Actor& actor) const {
        auto inv = actor.GetInventory([&](const RE::TESBoundObject& obj) { return &obj == form_; });
        auto it = inv.cbegin();
        if (it == inv.cend()) {
            return {};
        }
        const auto& [count, ied] = it->second;
        auto xls = tes_util::GetXLs(ied.get());
        auto count_excl_xl = count - tes_util::SumXLCounts(xls);

        std::erase_if(xls, [&](RE::ExtraDataList* xl) {
            return !xl || !MatchesXL(*xl, XLWornType::kAny);
        });
        // Prioritize tempering level, followed by enchant charges.
        std::sort(
            xls.begin(),
            xls.end(),
            [](RE::ExtraDataList* const a, RE::ExtraDataList* const b) {
                return tes_util::GetXLEnchCharge(a) > tes_util::GetXLEnchCharge(b);
            }
        );
        std::stable_sort(
            xls.begin(),
            xls.end(),
            [](RE::ExtraDataList* const a, RE::ExtraDataList* const b) {
                auto* a_xhp = a->GetByType<RE::ExtraHealth>();
                auto a_hp = a_xhp ? a_xhp->health : 1.f;
                auto* b_xhp = b->GetByType<RE::ExtraHealth>();
                auto b_hp = b_xhp ? b_xhp->health : 1.f;
                return a_hp > b_hp;
            }
        );

        auto new_count = tes_util::SumXLCounts(xls);
        auto matches_non_xl = extra().name.empty() && extra().ench == nullptr;
        if (matches_non_xl) {
            new_count += count_excl_xl;
        }
        return {new_count, std::move(xls)};
    }

    const RE::BGSEquipSlot*
    GetBGSEquipSlot() const {
        switch (slot()) {
            case Gearslot::kLeft:
                return tes_util::GetForm<RE::BGSEquipSlot>(tes_util::kEqupLeftHand);
            case Gearslot::kRight:
                return tes_util::GetForm<RE::BGSEquipSlot>(tes_util::kEqupRightHand);
            case Gearslot::kShout:
                return tes_util::GetForm<RE::BGSEquipSlot>(tes_util::kEqupVoice);
        }
        return nullptr;
    }

    explicit Gear(RE::TESForm* form, Gearslot slot, Extra extra)
        : form_(form),
          slot_(slot),
          extra_(std::move(extra)) {}

    RE::TESForm* form_;
    Gearslot slot_;
    Extra extra_;
};

/// Like a `std::variant<Gear, Gearslot>`.
class GearOrSlot final {
  public:
    GearOrSlot(Gear gear) : variant_(gear) {}

    GearOrSlot(Gearslot slot) : variant_(slot) {}

    /// Returns nullptr if this object is storing a `Gearslot`.
    const Gear*
    gear() const {
        return std::get_if<Gear>(&variant_);
    }

    Gearslot
    slot() const {
        if (const auto* g = gear()) {
            return g->slot();
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
    std::variant<Gear, Gearslot> variant_;
};

}  // namespace ech
