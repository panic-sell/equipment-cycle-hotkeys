// Utilities on top of CommonLibSSE.
#pragma once

template <>
struct fmt::formatter<RE::TESForm> {
    constexpr auto
    parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    auto
    format(const RE::TESForm& form, format_context& ctx) const {
        auto has_name = form.GetName() && *form.GetName();
        return fmt::format_to(
            ctx.out(),
            "{:08X}{}{}{}",
            form.GetFormID(),
            has_name ? " (" : "",
            has_name ? form.GetName() : "",
            has_name ? ")" : ""
        );
    }
};

namespace ech {
namespace tes_util {

inline constexpr RE::FormID kEqupRightHand = 0x13f42;
inline constexpr RE::FormID kEqupLeftHand = 0x13f43;
inline constexpr RE::FormID kEqupVoice = 0x25bee;
inline constexpr RE::FormID kWeapDummy = 0x20163;

/// Like `RE::TESForm::LookupByID()` but logs on failure.
inline RE::TESForm*
GetForm(RE::FormID form_id) {
    auto* form = RE::TESForm::LookupByID(form_id);
    if (!form) {
        SKSE::log::trace("unknown form {:08X}", form_id);
    }
    return form;
}

/// Like `RE::TESForm::LookupByID<T>()` but logs on failure.
template <typename T>
requires(std::is_base_of_v<RE::TESForm, T>)
T*
GetForm(RE::FormID form_id) {
    auto* form = GetForm(form_id);
    if (!form) {
        return nullptr;
    }
    auto* obj = form->As<T>();
    if (!obj) {
        SKSE::log::trace("{} cannot be cast to form type {}", *form, T::FORMTYPE);
    }
    return obj;
}

/// Like `RE::TESDataHandler::GetSingleton()->LookupForm()` but logs on failure.
///
/// Also supports looking up dynamic forms where there is no modname, in which case `local_id` is
/// treated as the full form ID.
inline RE::TESForm*
GetForm(std::string_view modname, RE::FormID local_id) {
    if (modname.empty()) {
        return GetForm(local_id);
    }

    auto* data_handler = RE::TESDataHandler::GetSingleton();
    if (!data_handler) {
        SKSE::log::error("cannot get RE::TESDataHandler instance");
        return nullptr;
    }
    auto* form = data_handler->LookupForm(local_id, modname);
    if (!form) {
        SKSE::log::trace("unknown form ({}, {:08X})", modname, local_id);
    }
    return form;
}

/// Like `RE::TESDataHandler::GetSingleton()->LookupForm<T>()` but logs on failure.
///
/// Also supports looking up dynamic forms where there is no modname, in which case, `local_id` is
/// treated as the full form ID.
template <typename T>
requires(std::is_base_of_v<RE::TESForm, T>)
T*
GetForm(std::string_view modname, RE::FormID local_id) {
    auto* form = GetForm(modname, local_id);
    if (!form) {
        return nullptr;
    }
    auto* obj = form->As<T>();
    if (!obj) {
        SKSE::log::trace("{} cannot be cast to form type {}", *form, T::FORMTYPE);
    }
    return obj;
}

/// Returns `(mod name, local ID)`.
///
/// If form is a dynamic form (e.g. a custom enchantment), returns `(empty string, full form ID)`.
inline std::pair<std::string_view, RE::FormID>
GetNamedFormID(const RE::TESForm& form) {
    const auto* file = form.GetFile(0);
    if (!file) {
        return {"", form.GetFormID()};
    }
    return {file->GetFilename(), form.GetLocalFormID()};
}

inline bool
IsVoiceEquippable(const RE::TESForm* form) {
    const auto* eqt = form ? form->As<RE::BGSEquipType>() : nullptr;
    const auto* slot = eqt ? eqt->GetEquipSlot() : nullptr;
    return slot && slot->GetFormID() == kEqupVoice;
}

inline bool
IsTwoHandedWeapon(const RE::TESForm* form) {
    const auto* weap = form ? form->As<RE::TESObjectWEAP>() : nullptr;
    // clang-format off
    return weap && (
        weap->IsTwoHandedAxe()
        || weap->IsTwoHandedSword()
        || weap->IsBow()
        || weap->IsCrossbow()
    );
    // clang-format on
}

inline bool
IsShield(const RE::TESForm* form) {
    const auto* armor = form ? form->As<RE::TESObjectARMO>() : nullptr;
    return armor && armor->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kShield);
}

/// Gets all extra lists from `ied`. All returned extra lists are guaranteed to be non-null.
inline std::vector<RE::ExtraDataList*>
GetXLs(const RE::InventoryEntryData* ied) {
    if (!ied || !ied->extraLists) {
        return {};
    }
    auto v = std::vector<RE::ExtraDataList*>();
    for (auto* xl : *ied->extraLists) {
        if (xl) {
            v.push_back(xl);
        }
    }
    return v;
}

inline int32_t
SumXLCounts(std::span<RE::ExtraDataList* const> xls) {
    int32_t c = 0;
    for (auto* xl : xls) {
        c += xl ? xl->GetCount() : 0;
    }
    return c;
}

inline float
GetXLHealth(RE::ExtraDataList* xl) {
    auto* xhealth = xl ? xl->GetByType<RE::ExtraHealth>() : nullptr;
    return xhealth ? xhealth->health : 1.f;
}

inline float
GetXLEnchCharge(RE::ExtraDataList* xl) {
    auto* xench = xl ? xl->GetByType<RE::ExtraEnchantment>() : nullptr;
    if (!xench) {
        return -std::numeric_limits<float>::infinity();
    }
    auto* xcharge = xl->GetByType<RE::ExtraCharge>();
    return xcharge ? xcharge->charge : static_cast<float>(xench->charge);
}

/// Returns false on failing to acquire the necessary resources.
template <class... Args>
[[nodiscard]] inline bool
SetSubtitle(fmt::format_string<Args...> fmt, Args&&... args) {
    auto subtitle = fmt::vformat(fmt.get(), fmt::make_format_args(args...));

    auto* q = RE::UIMessageQueue::GetSingleton();
    if (!q) {
        return false;
    }

    auto* facman = RE::MessageDataFactoryManager::GetSingleton();
    auto* istrs = RE::InterfaceStrings::GetSingleton();
    auto* fac = facman && istrs ? facman->GetCreator<RE::HUDData>(istrs->hudData) : nullptr;
    auto* huddata = fac ? fac->Create() : nullptr;
    if (!huddata) {
        return false;
    }

    huddata->type = RE::HUDData::Type::kSubtitle;
    huddata->text = std::move(subtitle);
    q->AddMessage(RE::HUDMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, huddata);
    return true;
}

/// Clears `actor`'s subtitles if `subtitle` is the empty string.
///
/// The caller is expected to invoke `stm->lock.Lock()` and `stm->lock.Unlock()` before and after
/// this function.
inline void
SetSubtitle(RE::SubtitleManager& stm, RE::Actor& actor, std::string_view subtitle) {
    auto staging = RE::BSTArray<RE::SubtitleInfo>();
    for (auto& info : stm.subtitles) {
        if (info.speaker != actor.GetHandle()) {
            staging.push_back(std::move(info));
        }
    }
    if (!subtitle.empty()) {
        staging.push_back(RE::SubtitleInfo{
            .speaker = actor.GetHandle(),
            .subtitle = subtitle,
        });
    }

    stm.KillSubtitles();
    stm.subtitles = std::move(staging);
}

}  // namespace tes_util
}  // namespace ech
