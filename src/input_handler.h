#pragma once

#include "equipsets.h"
#include "hotkeys.h"
#include "keys.h"
#include "tes_util.h"

namespace ech {
namespace internal {

/// Dev utility function. Used in combination with a debugger to inspect player's currently equipped
/// items.
inline void
InspectEquipped(std::span<const Keystroke> keystrokes, RE::Actor& actor) {
    static const auto keysets = Keysets({{KeycodeFromName("."), KeycodeFromName(";")}});
    if (keysets.Match(keystrokes) != Keypress::kPress) {
        return;
    }

    struct SlotData final {
        std::optional<Gear> gear;
        RE::TESForm* form = nullptr;
        RE::InventoryEntryData* ied = nullptr;

        RE::TESObjectWEAP* obj_weap = nullptr;
        RE::TESObjectARMO* obj_shield = nullptr;
        RE::SpellItem* obj_spell = nullptr;
        RE::ScrollItem* obj_scroll = nullptr;
        RE::TESAmmo* obj_ammo = nullptr;
        RE::TESShout* obj_shout = nullptr;

        SlotData(std::optional<Gear> gear, RE::TESForm* form, RE::InventoryEntryData* ied = nullptr)
            : gear(gear),
              form(form),
              ied(ied) {
            if (!form) {
                return;
            }
            obj_weap = form->As<RE::TESObjectWEAP>();
            obj_shield = form->As<RE::TESObjectARMO>();
            obj_spell = form->As<RE::SpellItem>();
            obj_scroll = form->As<RE::ScrollItem>();
            obj_ammo = form->As<RE::TESAmmo>();
            obj_shout = form->As<RE::TESShout>();
        }
    };

    auto sd_left = SlotData(
        Gear::FromEquipped(actor, Gearslot::kLeft),
        actor.GetEquippedObject(true),
        actor.GetEquippedEntryData(true)
    );
    auto sd_right = SlotData(
        Gear::FromEquipped(actor, Gearslot::kRight),
        actor.GetEquippedObject(false),
        actor.GetEquippedEntryData(false)
    );
    auto sd_ammo = SlotData(Gear::FromEquipped(actor, Gearslot::kAmmo), actor.GetCurrentAmmo());
    auto sd_shout = SlotData(
        Gear::FromEquipped(actor, Gearslot::kShout), actor.GetActorRuntimeData().selectedPower
    );

    (void)actor;  // breakpoint here
}

}  // namespace internal

/// Handles hotkey activations. Does not handle UI toggling; see InputHook instead.
class InputHandler final : public RE::BSTEventSink<RE::InputEvent*> {
  public:
    [[nodiscard]] static std::expected<void, std::string_view>
    Init(Hotkeys<>& hotkeys, std::mutex& hotkeys_mutex) {
        auto* idm = RE::BSInputDeviceManager::GetSingleton();
        if (!idm) {
            return std::unexpected("cannot get input event source");
        }

        static auto instance = InputHandler(hotkeys, hotkeys_mutex);
        idm->AddEventSink<RE::InputEvent*>(&instance);
        return {};
    }

    RE::BSEventNotifyControl
    ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override {
        HandleInputEvents(events);
        return RE::BSEventNotifyControl::kContinue;
    }

  private:
    InputHandler(Hotkeys<>& hotkeys, std::mutex& hotkeys_mutex)
        : RE::BSTEventSink<RE::InputEvent*>(),
          hotkeys_(&hotkeys),
          hotkeys_mutex_(&hotkeys_mutex) {}

    InputHandler(const InputHandler&) = delete;
    InputHandler& operator=(const InputHandler&) = delete;
    InputHandler(InputHandler&&) = delete;
    InputHandler& operator=(InputHandler&&) = delete;

    void
    HandleInputEvents(RE::InputEvent* const* events) {
        if (!events) {
            return;
        }

        auto* ui = RE::UI::GetSingleton();
        auto* control_map = RE::ControlMap::GetSingleton();
        if ( // clang-format off
            !ui
            || ui->GameIsPaused()
            || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)
            || ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME)
            || !control_map
            || !control_map->IsMovementControlsEnabled()
        ) {  // clang-format on
            return;
        }

        keystroke_buf_.clear();
        Keystroke::InputEventsToBuffer(*events, keystroke_buf_);
        if (keystroke_buf_.empty()) {
            return;
        }

        auto* aem = RE::ActorEquipManager::GetSingleton();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!aem || !player) {
            return;
        }

#ifndef NDEBUG
        internal::InspectEquipped(keystroke_buf_, *player);
#endif

        auto lock = std::lock_guard(*hotkeys_mutex_);
        const auto* orig = hotkeys_->GetSelectedEquipset();
        auto press_type = hotkeys_->SelectNextEquipset(keystroke_buf_);
        if (press_type == Keypress::kNone || press_type == Keypress::kSemihold) {
            return;
        }
        const auto* current = hotkeys_->GetSelectedEquipset();
        if (!current || (orig == current && press_type == Keypress::kHold)) {
            return;
        }
        current->Apply(*aem, *player);
        const auto& hkname = hotkeys_->vec()[hotkeys_->selected()].name;
        SKSE::log::debug(
            "selected hotkey {}{}{}{}{} equipset {}",
            hotkeys_->selected() + 1,
            hkname.empty() ? "" : " ",
            hkname.empty() ? "" : "(",
            hkname.empty() ? "" : hkname.c_str(),
            hkname.empty() ? "" : ")",
            hotkeys_->vec()[hotkeys_->selected()].equipsets.selected() + 1
        );
    }

    Hotkeys<>* hotkeys_;
    std::mutex* hotkeys_mutex_;
    /// Reusable buffer for storing input keystrokes and avoiding per-input-event allocations. We
    /// assume `HandleInputEvents()` will only be called from one thread at a time.
    std::vector<Keystroke> keystroke_buf_;
};

}  // namespace ech
