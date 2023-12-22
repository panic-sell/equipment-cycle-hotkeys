#pragma once

#include "keys.h"

namespace ech {

/// Global settings.
struct Settings final {
    float font_scale = 1.f;
    uint8_t color_style = 0;
    Keysets menu_toggle_keysets = Keysets({
        {KeycodeFromName("LShift"), KeycodeFromName("=")},
        {KeycodeFromName("RShift"), KeycodeFromName("=")},
    });

    static const Settings&
    GetSingleton(Settings* settings = nullptr) {
        static std::optional<Settings> instance;
        if (instance) {
            return *instance;
        }
        if (settings) {
            instance.emplace(std::move(*settings));
        } else {
            instance.emplace();
        }
        return *instance;
    }
};

}  // namespace ech
