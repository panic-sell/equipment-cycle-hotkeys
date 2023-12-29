#pragma once

#include "equipsets.h"
#include "keys.h"

namespace ech {

template <typename Q = Equipset>
struct Hotkey final {
    std::string name;
    Keysets keysets;
    Equipsets<Q> equipsets;
};

/// An ordered collection of 0 or more hotkeys.
///
/// The "selected" hotkey is the one that most recently matched key inputs.
///
/// Invariants:
/// - All hotkeys have at least 1 keyset or at least 1 equipset.
/// - `selected_ == size_t(-1)` means no hotkeys are selected.
/// - `most_recent_next_equipset_ == nullptr` if no hotkeys are selected.
///
/// This class is templated by "equipset" to facilitate unit testing. We swap out the real Equipset
/// type so that tests don't depend on Skyrim itself.
template <typename Q = Equipset>
class Hotkeys final {
  public:
    Hotkeys() = default;

    /// `initial_selection` applies AFTER pruning hotkeys.
    explicit Hotkeys(std::vector<Hotkey<Q>> hotkeys, size_t initial_selection = -1)
        : hotkeys_(std::move(hotkeys)),
          selected_(initial_selection) {
        std::erase_if(hotkeys_, [](const Hotkey<Q>& hk) {
            return hk.keysets.vec().empty() && hk.equipsets.vec().empty();
        });
        if (selected_ >= hotkeys_.size()) {
            Deselect();
        }
    }

    const std::vector<Hotkey<Q>>&
    vec() const {
        return hotkeys_;
    }

    /// Returns the selected hotkey's index.
    size_t
    selected() const {
        return selected_;
    }

    /// Ensures no hotkey is selected. Note that this does not change any hotkey's "selected
    /// equipset".
    void
    Deselect() {
        selected_ = size_t(-1);
        most_recent_next_equipset_ = nullptr;
    }

    /// Returns the selected hotkey's selected equipset. Returns nullptr if:
    /// - No hotkey is selected.
    /// - The selected hotkey has no equipsets.
    const Q*
    GetSelectedEquipset() const {
        if (selected_ >= hotkeys_.size()) {
            return nullptr;
        }
        const Hotkey<Q>& hk = hotkeys_[selected_];
        return hk.equipsets.GetSelected();
    }

    /// Selects the first hotkey that has at least one equipset and matches `keystrokes`, then
    /// returns that hotkey's next selected equipset. A hotkey's next selected equipset is
    /// determined as follows:
    /// - If the matching `keystroke` is a hold, select the hotkey's first equipset.
    /// - If the matching `keystroke` is a press and hotkey was already selected, call
    /// `hotkey.equipset.SelectNext()`, then return `hotkey.equipsets.GetSelected()`.
    /// - If the matching `keystroke` is a press and hotkey was not already selected, returns
    /// `hotkey.equipsets.GetSelected()`.
    ///
    /// A non-null return value means "player should equip what was returned".
    ///
    /// Returns nullptr if:
    /// - `keytrokes` is empty.
    /// - There are no hotkeys.
    /// - No hotkey matches `keystrokes`.
    /// - The matched hotkey has no equipsets.
    /// - The matched hotkey's match result is a semihold.
    /// - This function would have returned Q, where Q is the most recent non-null equipset returned
    /// by a prior call of this function.
    const Q*
    SelectNextEquipset(std::span<const Keystroke> keystrokes) {
        if (keystrokes.empty()) {
            return nullptr;
        }

        auto match_res = Keysets::MatchResult::kNone;
        auto it = std::find_if(hotkeys_.begin(), hotkeys_.end(), [&](const Hotkey<Q>& hotkey) {
            if (hotkey.equipsets.vec().empty()) {
                return false;
            }
            match_res = hotkey.keysets.Match(keystrokes);
            return match_res != Keysets::MatchResult::kNone;
        });
        if (it == hotkeys_.end() || match_res == Keysets::MatchResult::kSemihold) {
            return nullptr;
        }

        Hotkey<Q>& hk = *it;
        auto orig_selected = selected_;
        selected_ = it - hotkeys_.begin();

        if (match_res == Keysets::MatchResult::kHold) {
            hk.equipsets.SelectFirst();
        } else if (selected_ == orig_selected) {
            hk.equipsets.SelectNext();
        }
        auto next = hk.equipsets.GetSelected();

        // When resetting a hotkey's selected equipset, this prevents returning the first equipset
        // over and over again when input buttons are kept held down.
        if (next == most_recent_next_equipset_) {
            return nullptr;
        }
        most_recent_next_equipset_ = next;
        return next;
    }

  private:
    std::vector<Hotkey<Q>> hotkeys_;
    size_t selected_ = size_t(-1);
    const Q* most_recent_next_equipset_ = nullptr;
};

}  // namespace ech
