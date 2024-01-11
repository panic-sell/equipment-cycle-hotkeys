#pragma once

#include "equipsets.h"
#include "hotkeys.h"
#include "keys.h"
#include "tes_util.h"

namespace ech {
namespace internal {

inline void
DebugInspectEquipped(std::span<const Keystroke> keystrokes) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    static const auto keysets = Keysets({{
        KeycodeFromName("NumpadEnter"),
        KeycodeFromName("Numpad1"),
    }});
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
        Gear::FromEquipped(*player, Gearslot::kLeft),
        player->GetEquippedObject(true),
        player->GetEquippedEntryData(true)
    );
    auto sd_right = SlotData(
        Gear::FromEquipped(*player, Gearslot::kRight),
        player->GetEquippedObject(false),
        player->GetEquippedEntryData(false)
    );
    auto sd_ammo = SlotData(Gear::FromEquipped(*player, Gearslot::kAmmo), player->GetCurrentAmmo());
    auto sd_shout = SlotData(
        Gear::FromEquipped(*player, Gearslot::kShout), player->GetActorRuntimeData().selectedPower
    );

    (void)player;  // breakpoint here
}

inline bool
AcceptingInput() {
    auto* ui = RE::UI::GetSingleton();
    if (!ui || ui->GameIsPaused() || ui->IsMenuOpen("LootMenu")) {
        return false;
    }

    const auto* control_map = RE::ControlMap::GetSingleton();
    if (!control_map || !control_map->IsFightingControlsEnabled()) {
        return false;
    }

    const auto& cmstack = control_map->GetRuntimeData().contextPriorityStack;
    if (cmstack.empty() || cmstack.back() != RE::UserEvents::INPUT_CONTEXT_ID::kGameplay) {
        return false;
    }

    return true;
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
          hotkeys_(hotkeys),
          hotkeys_mutex_(hotkeys_mutex) {}

    InputHandler(const InputHandler&) = delete;
    InputHandler& operator=(const InputHandler&) = delete;
    InputHandler(InputHandler&&) = delete;
    InputHandler& operator=(InputHandler&&) = delete;

    void
    HandleInputEvents(RE::InputEvent* const* events) {
        if (!events) {
            return;
        }

        buf_.clear();
        Keystroke::InputEventsToBuffer(*events, buf_);
        if (buf_.empty()) {
            return;
        }

#ifndef NDEBUG
        internal::DebugInspectEquipped(buf_);
#endif

        if (!internal::AcceptingInput()) {
            return;
        }

        auto* aem = RE::ActorEquipManager::GetSingleton();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!aem || !player) {
            return;
        }

        auto lock = std::lock_guard(hotkeys_mutex_);
        const auto* orig = hotkeys_.GetSelectedEquipset();
        auto press_type = hotkeys_.SelectNextEquipset(buf_);
        if (press_type == Keypress::kNone || press_type == Keypress::kSemihold) {
            return;
        }
        const auto* current = hotkeys_.GetSelectedEquipset();
        if (!current || (orig == current && press_type == Keypress::kHold)) {
            return;
        }
        current->Apply(*aem, *player);
        const auto& hkname = hotkeys_.vec()[hotkeys_.selected()].name;
        SKSE::log::debug(
            "selected hotkey {}{}{}{}{} equipset {}",
            hotkeys_.selected() + 1,
            hkname.empty() ? "" : " ",
            hkname.empty() ? "" : "(",
            hkname.empty() ? "" : hkname.c_str(),
            hkname.empty() ? "" : ")",
            hotkeys_.vec()[hotkeys_.selected()].equipsets.selected() + 1
        );
    }

    Hotkeys<>& hotkeys_;
    std::mutex& hotkeys_mutex_;
    /// Reusable buffer for storing input keystrokes and avoiding per-input-event allocations. We
    /// assume `HandleInputEvents()` will only be called from one thread at a time.
    std::vector<Keystroke> buf_;
};

}  // namespace ech
