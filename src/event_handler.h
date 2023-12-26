#pragma once

#include "dev_util.h"
#include "equipsets.h"
#include "hotkeys.h"
#include "keys.h"
#include "tes_util.h"

namespace ech {

class EventHandler final : public RE::BSTEventSink<RE::InputEvent*>,
                           public RE::BSTEventSink<RE::TESEquipEvent> {
  public:
    static std::expected<void, std::string_view>
    Register(Hotkeys<>& hotkeys) {
        auto* idm = RE::BSInputDeviceManager::GetSingleton();
        auto* sesh = RE::ScriptEventSourceHolder::GetSingleton();
        if (!idm || !sesh) {
            return std::unexpected("failed to get event sources");
        }

        static EventHandler instance;
        instance.hotkeys_ = &hotkeys;
        idm->AddEventSink<RE::InputEvent*>(&instance);
        sesh->AddEventSink<RE::TESEquipEvent>(&instance);
        return {};
    }

    /// Triggers hotkey activations.
    RE::BSEventNotifyControl
    ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override {
        HandleInputEvents(events);
        return RE::BSEventNotifyControl::kContinue;
    }

    /// Deactivates hotkeys if something else equips/unequips player gear.
    RE::BSEventNotifyControl
    ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>*) override {
        HandleEquipEvent(event);
        return RE::BSEventNotifyControl::kContinue;
    }

  private:
    EventHandler() = default;
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;

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

        const auto* equipset =
            dev_util::input_handlers::UseHotkeys(*hotkeys_, keystroke_buf_, *player);
        if (equipset) {
            // Note the ordering here. Most-recent-equip-time must be reset prior to equipset-apply
            // because the latter will trigger equip events.
            most_recent_hotkey_equip_time_ = RE::GetDurationOfApplicationRunTime();
            equipset->Apply(*aem, *player);
        }
    }

    void
    HandleEquipEvent(const RE::TESEquipEvent* event) {
        if (!event || !event->actor || !event->actor->IsPlayerRef()) {
            return;
        }
        const auto* form = RE::TESForm::LookupByID(event->baseObject);
        if (!form) {
            return;
        }

        // Ignore equip/unequip actions on items that don't map to supported gear slots.
        switch (form->GetFormType()) {
            case RE::FormType::Armor:
                if (!tes_util::IsShield(form)) {
                    return;
                }
            case RE::FormType::Spell:
            case RE::FormType::Weapon:
            case RE::FormType::Light:
            case RE::FormType::Ammo:
            case RE::FormType::Shout:
                break;
            default:
                return;
        }

        auto now = RE::GetDurationOfApplicationRunTime();
        if (now >= most_recent_hotkey_equip_time_ + 500) {
            hotkeys_->Deactivate();
        }
    }

    Hotkeys<>* hotkeys_ = nullptr;
    /// Reusable buffer for storing input keystrokes and avoiding per-input-event allocations.
    std::vector<Keystroke> keystroke_buf_;
    /// In milliseconds since application start.
    uint32_t most_recent_hotkey_equip_time_ = 0;
};

}  // namespace ech
