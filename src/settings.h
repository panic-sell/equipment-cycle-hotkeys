#pragma once

#include "keys.h"

namespace ech {

/// Global settings.
struct Settings final {
    float menu_font_scale = 1.f;
    std::string menu_color_style = "Dark";
    Keysets menu_toggle_keysets = Keysets({
        {KeycodeFromName("LShift"), KeycodeFromName("=")},
        {KeycodeFromName("RShift"), KeycodeFromName("=")},
    });
};

}  // namespace ech
