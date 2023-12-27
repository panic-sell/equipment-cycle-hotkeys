#pragma once

#include "keys.h"

namespace ech {

/// Global settings.
struct Settings final {
    std::string log_level = "info";
    float menu_font_scale = 1.f;
    std::string menu_color_style = "dark";
    Keysets menu_toggle_keysets = Keysets({
        {KeycodeFromName("LShift"), KeycodeFromName("=")},
        {KeycodeFromName("RShift"), KeycodeFromName("=")},
    });
};

}  // namespace ech
