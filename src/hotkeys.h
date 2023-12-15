#pragma once

#include "equipsets.h"
#include "keys.h"

namespace ech {

template <typename T = Equipset>
struct Hotkey final {
    std::string name;
    Keysets keysets;
    Equipsets<T> equipsets;
};

/// An ordered collection of 0 or more hotkeys.
///
/// The "active" hotkey is the one that most recently matched key inputs.
///
/// Invariants:
/// - All hotkeys have at least 1 keyset or at least 1 equipset.
/// - `active_ == hotkeys_.size()` means no hotkeys are active.
/// - `most_recent_next_equipset_ == nullptr` if no hotkeys are active.
///
/// This class is templated by "equipset" to facilitate unit testing. We swap out the real Equipset
/// type so that tests don't depend on Skyrim itself.
template <typename T = Equipset>
class Hotkeys final {
  public:
    Hotkeys() = default;

    /// `active_index` applies AFTER pruning hotkeys.
    explicit Hotkeys(std::vector<Hotkey<T>> hotkeys, size_t active_index = -1)
        : hotkeys_(std::move(hotkeys)),
          active_(active_index) {
        std::erase_if(hotkeys_, [](const Hotkey<T>& hk) {
            return hk.keysets.vec().empty() && hk.equipsets.vec().empty();
        });
        if (active_ >= hotkeys_.size()) {
            Deactivate();
        }
    }

    const std::vector<Hotkey<T>>&
    vec() const {
        return hotkeys_;
    }

    /// Returns the active hotkey's index.
    size_t
    active() const {
        return active_;
    }

    /// Make none of the hotkeys active.
    void
    Deactivate() {
        active_ = hotkeys_.size();
        most_recent_next_equipset_ = nullptr;
    }

    /// Returns the active hotkey's active equipset. Returns nullptr if:
    /// - No hotkey is active.
    /// - The active hotkey has no equipsets.
    const T*
    GetActiveEquipset() const {
        if (active_ >= hotkeys_.size()) {
            return nullptr;
        }
        const Hotkey<T>& hk = hotkeys_[active_];
        return hk.equipsets.GetActive();
    }

    /// Activates the first hotkey that has at least one equipset and matches `keystrokes`, then
    /// returns that hotkey's next active equipset. A non-null return value means "player should
    /// equip what was returned".
    ///
    /// Returns nullptr if:
    /// - `keytrokes` is empty.
    /// - There are no hotkeys.
    /// - No hotkey matches `keystrokes`.
    /// - The matched hotkey has no equipsets.
    /// - The matched hotkey's match result is a semihold.
    /// - This function would have returned E, where E is the most recent non-null equipset returned
    /// by a prior call of this function.
    const T*
    GetNextEquipset(std::span<const Keystroke> keystrokes) {
        if (keystrokes.empty()) {
            return nullptr;
        }

        auto match_res = Keysets::MatchResult::kNone;
        auto it = std::find_if(hotkeys_.begin(), hotkeys_.end(), [&](const Hotkey<T>& hotkey) {
            if (hotkey.equipsets.vec().empty()) {
                return false;
            }
            match_res = hotkey.keysets.Match(keystrokes);
            return match_res != Keysets::MatchResult::kNone;
        });
        if (it == hotkeys_.end() || match_res == Keysets::MatchResult::kSemihold) {
            return nullptr;
        }

        Hotkey<T>& hk = *it;
        auto orig_active = active_;
        active_ = it - hotkeys_.begin();

        if (match_res == Keysets::MatchResult::kHold) {
            hk.equipsets.ActivateFirst();
        } else if (active_ == orig_active) {
            hk.equipsets.ActivateNext();
        }
        auto next = hk.equipsets.GetActive();

        // When resetting a hotkey's active equipset, this prevents returning the first equipset
        // over and over again.
        if (next == most_recent_next_equipset_) {
            return nullptr;
        }
        most_recent_next_equipset_ = next;
        return next;
    }

  private:
    std::vector<Hotkey<T>> hotkeys_;
    size_t active_;
    const T* most_recent_next_equipset_ = nullptr;
};

}  // namespace ech
