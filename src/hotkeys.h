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
/// - `selected_ == SIZE_MAX` means no hotkeys are selected.
/// - Throughout an instance's lifetime, every contained equipset has a stable, distinct pointer.
///
/// This class is templated by "equipset" to facilitate unit testing. We swap out the real Equipset
/// type so that tests don't depend on Skyrim itself.
template <typename Q = Equipset>
class Hotkeys final {
  public:
    Hotkeys() = default;

    /// `initial_selection` applies AFTER pruning hotkeys.
    explicit Hotkeys(
        std::vector<Hotkey<Q>> hotkeys,
        size_t initial_selection = std::numeric_limits<size_t>::max()
    )
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
    Hotkeys&
    Deselect() {
        selected_ = std::numeric_limits<size_t>::max();
        return *this;
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
    /// selects an equipset within that hotkey.
    ///
    /// Choosing which of that hotkey's equipset to select is done as follows:
    /// - If the matching `keystroke` is a hold, select the hotkey's first equipset.
    /// - If the matching `keystroke` is a press and the hotkey was already selected, select the
    /// hotkey's next ordered equipset.
    /// - If the matching `keystroke` is a press and the hotkey was not already selected, don't
    /// change the selected equipset.
    Hotkeys&
    SelectNextEquipset(std::span<const Keystroke> keystrokes) {
        if (keystrokes.empty()) {
            return *this;
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
            return *this;
        }

        Hotkey<Q>& hk = *it;
        auto orig_selected = selected_;
        selected_ = it - hotkeys_.begin();

        if (match_res == Keysets::MatchResult::kHold) {
            hk.equipsets.SelectFirst();
        } else if (selected_ == orig_selected) {
            hk.equipsets.SelectNext();
        }

        return *this;
    }

    /// Checks for equality of names, keysets data, and equipset data. Ignores hotkey/equipset
    /// selection state.
    bool
    StructurallyEquals(const Hotkeys& other) const
    requires(std::equality_comparable<Q>)
    {
        if (vec().size() != other.vec().size()) {
            return false;
        }
        for (size_t i = 0; i < vec().size(); i++) {
            const Hotkey<Q>& a = vec()[i];
            const Hotkey<Q>& b = other.vec()[i];
            if (a.name != b.name) {
                return false;
            }
            if (a.keysets.vec() != b.keysets.vec()) {
                return false;
            }
            if (a.equipsets.vec() != b.equipsets.vec()) {
                return false;
            }
        }
        return true;
    }

  private:
    std::vector<Hotkey<Q>> hotkeys_;
    size_t selected_ = std::numeric_limits<size_t>::max();
};

}  // namespace ech
