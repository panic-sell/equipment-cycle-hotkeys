#pragma once

#include "hotkeys.h"
#include "keys.h"
#include "ui_drawing.h"

namespace ech {

/// Global settings.
struct Settings final {
    float menu_font_scale = 1.f;
    uint8_t menu_color_style = 0;
    Keysets menu_toggle_keysets = Keysets({
        {KeycodeFromName("LShift"), KeycodeFromName("=")},
        {KeycodeFromName("RShift"), KeycodeFromName("=")},
    });
};

}  // namespace ech
