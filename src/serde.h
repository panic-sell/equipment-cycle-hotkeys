#pragma once

#include "equipsets.h"
#include "hotkeys.h"
#include "keys.h"
#include "settings.h"
#include "tes_util.h"

namespace ech {
namespace internal {

/// Tries to get a field value from a serialized object. Returns nullopt if the field does not exist
/// or cannot be converted to `T`.
template <typename T, typename C>
inline std::optional<T>
GetSerObjField(const boost::json::object& jo, std::string_view name, const C& ctx) {
    const auto* jv = jo.if_contains(name);
    if (!jv) {
        return std::nullopt;
    }
    auto result = boost::json::try_value_to<T>(*jv, ctx);
    return result ? std::optional(std::move(*result)) : std::nullopt;
}

}  // namespace internal

/// Context for implementing Boost.JSON tag_invoke overloads, specifically for types in this
/// project.
///
/// Currently, this is only used to ensure that JSON conversions do NOT treat `Keyset` as a typical
/// `std::array<uint32_t, 4>`.
///
/// This class must be defined in the same namespace as classes and tag_invoke overloads.
struct SerdeContext final {};

/// Serializes object to compact JSON string.
///
/// Unlike `Deserialize()`, this function does not have a variant serializing `T` to
/// `boost::json::value` because `boost::json::value_from()` already serves that role.
template <typename T, typename C = SerdeContext>
inline std::string
Serialize(const T& t, const C& ctx = {}) {
    return boost::json::serialize(boost::json::value_from(t, ctx));
}

/// Deserializes `boost::json::value` from JSON string. Input is allowed to contain comment and
/// trailing commas.
inline std::optional<boost::json::value>
Deserialize(std::string_view s) {
    constexpr auto opts = boost::json::parse_options{
        .allow_comments = true,
        .allow_trailing_commas = true,
    };

    std::error_code ec;
    auto jv = boost::json::parse(s, ec, {}, opts);
    return !ec ? std::optional(std::move(jv)) : std::nullopt;
}

/// Deserializes object from JSON string. Input is allowed to contain comment and trailing commas.
template <typename T, typename C = SerdeContext>
inline std::optional<T>
Deserialize(std::string_view s, const C& ctx = {}) {
    return Deserialize(s).and_then([&ctx](boost::json::value&& jv) {
        auto res = boost::json::try_value_to<T>(jv, ctx);
        return res ? std::optional(std::move(*res)) : std::nullopt;
    });
}

inline void
tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const Keyset& keyset, const SerdeContext&) {
    auto ja = boost::json::array();
    for (auto keycode : keyset) {
        if (KeycodeIsValid(keycode)) {
            auto s = boost::json::string_view(KeycodeName(keycode));
            ja.push_back(s);
        }
    }
    jv = std::move(ja);
}

inline boost::json::result<Keyset>
tag_invoke(
    const boost::json::try_value_to_tag<Keyset>&,
    const boost::json::value& jv,
    const SerdeContext& ctx
) {
    auto keyset = KeysetNormalized({});
    auto v = boost::json::try_value_to<std::vector<std::string>>(jv, ctx);
    if (!v) {
        return keyset;
    }

    auto sz = std::min(v->size(), keyset.size());
    for (size_t i = 0; i < sz; i++) {
        std::string_view name = (*v)[i];
        keyset[i] = KeycodeFromName(name);
    }
    return KeysetNormalized(keyset);
}

inline void
tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const Equipset& equipset, const SerdeContext&) {
    constexpr auto value_from_item = [](const GearOrSlot& item) -> boost::json::value {
        auto jo = boost::json::object();
        jo.insert_or_assign("slot", std::to_underlying(item.slot()));

        const auto* gear = item.gear();
        if (!gear) {
            jo.insert_or_assign("unequip", true);
            return jo;
        }

        const auto& [mod, id] = tes_util::GetNamedFormID(gear->form());
        if (mod.empty() || id == 0) {
            return {};
        }
        if (!mod.empty()) {
            jo.insert_or_assign("mod", mod);
        }
        if (id != 0) {
            jo.insert_or_assign("id", id);
        }
        if (std::isfinite(gear->extra_health())) {
            jo.insert_or_assign("extra_health", gear->extra_health());
        }
        if (gear->extra_ench()) {
            const auto& [ee_mod, ee_id] = tes_util::GetNamedFormID(*gear->extra_ench());
            if (!ee_mod.empty()) {
                jo.insert_or_assign("extra_ench_mod", ee_mod);
            }
            if (ee_id != 0) {
                jo.insert_or_assign("extra_ench_id", ee_id);
            }
        }

        return jo;
    };

    auto ja = boost::json::array();
    std::transform(
        equipset.vec().cbegin(), equipset.vec().cend(), std::back_inserter(ja), value_from_item
    );
    jv = std::move(ja);
}

inline boost::json::result<Equipset>
tag_invoke(
    const boost::json::try_value_to_tag<Equipset>&,
    const boost::json::value& jv,
    const SerdeContext& ctx
) {
    constexpr auto value_to_item = [](const boost::json::value& jv,
                                      const SerdeContext& ctx) -> std::optional<GearOrSlot> {
        if (!jv.is_object()) {
            return std::nullopt;
        }
        const auto& jo = jv.get_object();

        auto slot_num = internal::GetSerObjField<std::underlying_type_t<Gearslot>>(jo, "slot", ctx);
        if (!slot_num || *slot_num > std::to_underlying(Gearslot::MAX)) {
            return std::nullopt;
        }
        auto slot = static_cast<Gearslot>(*slot_num);

        auto unequip = internal::GetSerObjField<bool>(jo, "unequip", ctx);
        if (!unequip) {
            return std::nullopt;
        }
        if (*unequip) {
            return GearOrSlot(static_cast<Gearslot>(*slot_num));
        }

        auto mod = internal::GetSerObjField<std::string>(jo, "mod", ctx).value_or(""s);
        auto id = internal::GetSerObjField<RE::FormID>(jo, "id", ctx).value_or(0);
        auto* form = tes_util::GetForm(mod, id);
        if (!form) {
            return std::nullopt;
        }

        auto extra_health = internal::GetSerObjField<float>(jo, "extra_health", ctx)
                                .value_or(Gear::kExtraHealthNone);

        RE::EnchantmentItem* extra_ench = nullptr;
        auto ee_mod =
            internal::GetSerObjField<std::string>(jo, "extra_ench_mod", ctx).value_or(""s);
        auto ee_id = internal::GetSerObjField<RE::FormID>(jo, "extra_ench_id", ctx).value_or(0);
        if (!ee_mod.empty() || ee_id != 0) {
            extra_ench = tes_util::GetForm<RE::EnchantmentItem>(ee_mod, ee_id);
        }

        return Gear::New(form, slot == Gearslot::kLeft, extra_health, extra_ench);
    };

    if (!jv.is_array()) {
        return Equipset();
    }

    auto items = std::vector<GearOrSlot>();
    for (const auto& jitem : jv.get_array()) {
        auto item = value_to_item(jitem, ctx);
        if (!item) {
            // For consistency with other list-like classes, if any element is not a valid equipset
            // item, discard the entire JSON array.
            return Equipset();
        }
        items.push_back(*item);
    }
    return Equipset(std::move(items));
}

template <typename Q>
inline void
tag_invoke(
    const boost::json::value_from_tag&,
    boost::json::value& jv,
    const Hotkey<Q>& hotkey,
    const SerdeContext& ctx
) {
    auto jo = boost::json::object();
    if (!hotkey.name.empty()) {
        jo.insert_or_assign("name", hotkey.name);
    }
    if (!hotkey.keysets.vec().empty()) {
        jo.insert_or_assign("keysets", boost::json::value_from(hotkey.keysets.vec(), ctx));
    }
    if (!hotkey.equipsets.vec().empty()) {
        auto active_equipset = hotkey.equipsets.active();
        if (active_equipset > 0) {
            jo.insert_or_assign("active_equipset", active_equipset);
        }
        jo.insert_or_assign("equipsets", boost::json::value_from(hotkey.equipsets.vec(), ctx));
    }
    jv = std::move(jo);
}

template <typename Q>
inline boost::json::result<Hotkey<Q>>
tag_invoke(
    const boost::json::try_value_to_tag<Hotkey<Q>>&,
    const boost::json::value& jv,
    const SerdeContext& ctx
) {
    Hotkey<Q> hotkey;
    if (!jv.is_object()) {
        return hotkey;
    }
    const auto& jo = jv.get_object();

    hotkey.name = internal::GetSerObjField<std::string>(jo, "name", ctx).value_or("");
    hotkey.keysets = Keysets(internal::GetSerObjField<std::vector<Keyset>>(jo, "keysets", ctx)
                                 .value_or(std::vector<Keyset>()));

    auto active_equipset = internal::GetSerObjField<size_t>(jo, "active_equipset", ctx).value_or(0);
    hotkey.equipsets = Equipsets(
        internal::GetSerObjField<std::vector<Q>>(jo, "equipsets", ctx).value_or(std::vector<Q>()),
        active_equipset
    );

    return hotkey;
}

template <typename Q>
inline void
tag_invoke(
    const boost::json::value_from_tag&,
    boost::json::value& jv,
    const Hotkeys<Q>& hotkeys,
    const SerdeContext& ctx
) {
    auto jo = boost::json::object();
    if (hotkeys.active() < hotkeys.vec().size()) {
        jo.insert_or_assign("active_hotkey", hotkeys.active());
    }
    if (!hotkeys.vec().empty()) {
        jo.insert_or_assign("hotkeys", boost::json::value_from(hotkeys.vec(), ctx));
    }
    jv = std::move(jo);
}

template <typename Q>
inline boost::json::result<Hotkeys<Q>>
tag_invoke(
    const boost::json::try_value_to_tag<Hotkeys<Q>>&,
    const boost::json::value& jv,
    const SerdeContext& ctx
) {
    if (!jv.is_object()) {
        return Hotkeys<Q>();
    }
    const auto& jo = jv.get_object();

    auto active_hotkey = internal::GetSerObjField<size_t>(jo, "active_hotkey", ctx).value_or(-1);
    auto hotkeys = internal::GetSerObjField<std::vector<Hotkey<Q>>>(jo, "hotkeys", ctx)
                       .value_or(std::vector<Hotkey<Q>>());
    return Hotkeys<Q>(std::move(hotkeys), active_hotkey);
}

/// Note that there's no `value_from` tag_invoke. Settings are only every configured through JSON
/// files, so there's no need to serialize settings to JSON.
inline boost::json::result<Settings>
tag_invoke(
    const boost::json::try_value_to_tag<Settings>&,
    const boost::json::value& jv,
    const SerdeContext& ctx
) {
    auto settings = Settings();
    if (!jv.is_object()) {
        return settings;
    }
    auto jo = jv.get_object();

    settings.font_scale =
        internal::GetSerObjField<float>(jo, "font_scale", ctx).value_or(settings.font_scale);
    settings.color_style =
        internal::GetSerObjField<uint8_t>(jo, "color_style", ctx).value_or(settings.color_style);
    settings.menu_toggle_keysets =
        Keysets(internal::GetSerObjField<std::vector<Keyset>>(jo, "menu_toggle_keysets", ctx)
                    .value_or(settings.menu_toggle_keysets.vec()));
    return settings;
}

}  // namespace ech
