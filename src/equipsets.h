#pragma once

#include "gear.h"
#include "tes_util.h"

namespace ech {

/// A collection of to-be-equipped gear and to-be-unequipped slots.
///
/// Invariants:
/// - Items are sorted based on `GetActuationIndex()`.
/// - No two items share the same gear slot.
class Equipset final {
  public:
    Equipset() = default;
    bool operator==(const Equipset&) const = default;

    explicit Equipset(std::vector<GearOrSlot> items) : items_(std::move(items)) {
        std::stable_sort(
            items_.begin(),
            items_.end(),
            [](const GearOrSlot& a, const GearOrSlot& b) { return a.slot() < b.slot(); }
        );
        items_.erase(
            std::unique(
                items_.begin(),
                items_.end(),
                [](const GearOrSlot& a, const GearOrSlot& b) { return a.slot() == b.slot(); }
            ),
            items_.end()
        );
        std::sort(items_.begin(), items_.end(), [](const GearOrSlot& a, const GearOrSlot& b) {
            return GetActuationIndex(a) < GetActuationIndex(b);
        });
    }

    static Equipset
    FromEquipped(RE::Actor& actor, bool unequip_empty_slots = false) {
        std::vector<GearOrSlot> items;
        for (auto slot : kGearslots) {
            auto gear = Gear::FromEquipped(actor, slot);
            if (gear) {
                items.emplace_back(*gear);
            } else if (unequip_empty_slots) {
                items.emplace_back(slot);
            }
        }
        return Equipset(std::move(items));
    }

    const std::vector<GearOrSlot>&
    vec() const {
        return items_;
    }

    void
    Apply(RE::ActorEquipManager& aem, RE::Actor& actor) const {
        for (const auto& item : items_) {
            const auto* gear = item.gear();
            if (gear) {
                gear->Equip(aem, actor);
            } else {
                UnequipGear(aem, actor, item.slot());
            }
        }
    }

  private:
    /// Higher number means later actuation and taking precedence over preceding items.
    ///
    /// In general, the only hard requirements are that:
    /// 1. Unequip-left must precede equip-right because unequip-left removes 2h gear.
    /// 1. Equip-right must precede unequip-ammo because equipping a bow/crossbow auto equips ammo.
    static int
    GetActuationIndex(const GearOrSlot& item) {
        if (item.gear()) {
            switch (item.slot()) {
                case Gearslot::kLeft:
                    return 0;
                case Gearslot::kRight:
                    return 10;
                case Gearslot::kAmmo:
                    return 11;
                case Gearslot::kShout:
                    return 12;
            }
        } else {
            switch (item.slot()) {
                case Gearslot::kLeft:
                    return 1;
                case Gearslot::kRight:
                    return 20;
                case Gearslot::kAmmo:
                    return 21;
                case Gearslot::kShout:
                    return 22;
            }
        }
        return 99;
    }

    std::vector<GearOrSlot> items_;
};

/// An ordered collection of 0 or more equipsets.
///
/// Invariants:
/// - If `equipsets_.empty()`, then `selected_ == 0`.
/// - If `!equipsets_.empty()`, then `selected_ < equipsets.size()`. In other words, there is
/// always a selected equipset.
template <typename Q = Equipset>
class Equipsets final {
  public:
    Equipsets() = default;

    explicit Equipsets(std::vector<Q> equipsets, size_t initial_selection = 0)
        : equipsets_(std::move(equipsets)),
          selected_(initial_selection) {
        // Prune empty equipsets. An empty equipset ignores all gear slots, so cycling into one
        // gives no user feedback, which could be confusing.
        if constexpr (std::is_same_v<Q, Equipset>) {
            std::erase_if(equipsets_, [](const Equipset& equipset) {
                return equipset.vec().empty();
            });
        }
        if (selected_ >= equipsets_.size()) {
            SelectFirst();
        }
    }

    const std::vector<Q>&
    vec() const {
        return equipsets_;
    }

    /// Returns the index of the selected equipset.
    size_t
    selected() const {
        return selected_;
    }

    /// Returns a pointer to the selected equipset. Returns nullptr if there are no equipsets (if
    /// there exists at least 1 equipset, the return value is guaranteed to be non-null).
    const Q*
    GetSelected() const {
        if (equipsets_.empty()) {
            return nullptr;
        }
        auto i = selected_ < equipsets_.size() ? selected_ : 0;
        return &equipsets_[i];
    }

    void
    SelectFirst() {
        selected_ = 0;
    }

    void
    SelectNext() {
        if (!equipsets_.empty()) {
            selected_ = (selected_ + 1) % equipsets_.size();
        }
    }

  private:
    std::vector<Q> equipsets_;
    size_t selected_ = 0;
};

}  // namespace ech
