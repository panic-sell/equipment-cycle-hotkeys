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
    static const auto keysets = Keysets({{KeycodeFromName("/"), KeycodeFromName("'")}});
    if (keysets.Match(keystrokes) != Keysets::MatchResult::kPress) {
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
}

}  // namespace internal

class EventHandler final : public RE::BSTEventSink<RE::InputEvent*>,
                           public RE::BSTEventSink<RE::TESEquipEvent> {
  public:
    [[nodiscard]] static std::expected<void, std::string_view>
    Init(Hotkeys<>& hotkeys, std::mutex& hotkeys_mutex) {
        auto* idm = RE::BSInputDeviceManager::GetSingleton();
        auto* sesh = RE::ScriptEventSourceHolder::GetSingleton();
        if (!idm || !sesh) {
            return std::unexpected("failed to get event sources");
        }

        static EventHandler instance;
        instance.hotkeys_ = &hotkeys;
        instance.hotkeys_mutex_ = &hotkeys_mutex;
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

        internal::InspectEquipped(keystroke_buf_, *player);

        auto lock = std::lock_guard(*hotkeys_mutex_);
        const auto* equipset = hotkeys_->GetNextEquipset(keystroke_buf_);
        if (!equipset) {
            return;
        }

        // Most-recent-equip-time must be reset prior to equipset-apply because the latter
        // triggers equip events.
        auto now = RE::GetDurationOfApplicationRunTime();
        most_recent_hotkey_equip_time_.store(now);
        equipset->Apply(*aem, *player);
        SKSE::log::debug(
            "activated hotkey {} ({}), equipset {}",
            hotkeys_->active() + 1,
            hotkeys_->vec()[hotkeys_->active()].name,
            hotkeys_->vec()[hotkeys_->active()].equipsets.active() + 1
        );
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

        // Assume any equip event within 0.5 s of a hotkey activation is the result of that hotkey
        // activation.
        auto now = RE::GetDurationOfApplicationRunTime();
        if (now <= most_recent_hotkey_equip_time_.load() + 500) {
            return;
        }
        auto lock = std::lock_guard(*hotkeys_mutex_);
        hotkeys_->Deactivate();
        SKSE::log::debug("deactivated hotkeys");
    }

    Hotkeys<>* hotkeys_ = nullptr;
    std::mutex* hotkeys_mutex_ = nullptr;
    /// Reusable buffer for storing input keystrokes and avoiding per-input-event allocations. We
    /// assume `HandleInputEvents()` will only be called from one thread at a time.
    std::vector<Keystroke> keystroke_buf_;
    /// In milliseconds since application start.
    std::atomic<uint32_t> most_recent_hotkey_equip_time_ = 0;
};

}  // namespace ech
