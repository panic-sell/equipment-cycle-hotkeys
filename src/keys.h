#pragma once

namespace ech {

// https://ck.uesp.net/wiki/Input_Script
inline constexpr auto kKeycodeNames = std::array{
    "",              // 0
    "Esc",           // 1
    "1",             // 2
    "2",             // 3
    "3",             // 4
    "4",             // 5
    "5",             // 6
    "6",             // 7
    "7",             // 8
    "8",             // 9
    "9",             // 10
    "0",             // 11
    "-",             // 12
    "=",             // 13
    "Backspace",     // 14
    "Tab",           // 15
    "Q",             // 16
    "W",             // 17
    "E",             // 18
    "R",             // 19
    "T",             // 20
    "Y",             // 21
    "U",             // 22
    "I",             // 23
    "O",             // 24
    "P",             // 25
    "[",             // 26
    "]",             // 27
    "Enter",         // 28
    "LCtrl",         // 29
    "A",             // 30
    "S",             // 31
    "D",             // 32
    "F",             // 33
    "G",             // 34
    "H",             // 35
    "J",             // 36
    "K",             // 37
    "L",             // 38
    ";",             // 39
    "'",             // 40
    "`",             // 41
    "LShift",        // 42
    "\\",            // 43
    "Z",             // 44
    "X",             // 45
    "C",             // 46
    "V",             // 47
    "B",             // 48
    "N",             // 49
    "M",             // 50
    ",",             // 51
    ".",             // 52
    "/",             // 53
    "RShift",        // 54
    "Numpad*",       // 55
    "LAlt",          // 56
    "Space",         // 57
    "CapsLock",      // 58
    "F1",            // 59
    "F2",            // 60
    "F3",            // 61
    "F4",            // 62
    "F5",            // 63
    "F6",            // 64
    "F7",            // 65
    "F8",            // 66
    "F9",            // 67
    "F10",           // 68
    "NumLock",       // 69
    "ScrollLock",    // 70
    "Numpad7",       // 71
    "Numpad8",       // 72
    "Numpad9",       // 73
    "Numpad-",       // 74
    "Numpad4",       // 75
    "Numpad5",       // 76
    "Numpad6",       // 77
    "Numpad+",       // 78
    "Numpad1",       // 79
    "Numpad2",       // 80
    "Numpad3",       // 81
    "Numpad0",       // 82
    "Numpad.",       // 83
    "",              // 84
    "",              // 85
    "",              // 86
    "F11",           // 87
    "F12",           // 88
    "",              // 89
    "",              // 90
    "",              // 91
    "",              // 92
    "",              // 93
    "",              // 94
    "",              // 95
    "",              // 96
    "",              // 97
    "",              // 98
    "",              // 99
    "",              // 100
    "",              // 101
    "",              // 102
    "",              // 103
    "",              // 104
    "",              // 105
    "",              // 106
    "",              // 107
    "",              // 108
    "",              // 109
    "",              // 110
    "",              // 111
    "",              // 112
    "",              // 113
    "",              // 114
    "",              // 115
    "",              // 116
    "",              // 117
    "",              // 118
    "",              // 119
    "",              // 120
    "",              // 121
    "",              // 122
    "",              // 123
    "",              // 124
    "",              // 125
    "",              // 126
    "",              // 127
    "",              // 128
    "",              // 129
    "",              // 130
    "",              // 131
    "",              // 132
    "",              // 133
    "",              // 134
    "",              // 135
    "",              // 136
    "",              // 137
    "",              // 138
    "",              // 139
    "",              // 140
    "",              // 141
    "",              // 142
    "",              // 143
    "",              // 144
    "",              // 145
    "",              // 146
    "",              // 147
    "",              // 148
    "",              // 149
    "",              // 150
    "",              // 151
    "",              // 152
    "",              // 153
    "",              // 154
    "",              // 155
    "NumpadEnter",   // 156
    "RCtrl",         // 157
    "",              // 158
    "",              // 159
    "",              // 160
    "",              // 161
    "",              // 162
    "",              // 163
    "",              // 164
    "",              // 165
    "",              // 166
    "",              // 167
    "",              // 168
    "",              // 169
    "",              // 170
    "",              // 171
    "",              // 172
    "",              // 173
    "",              // 174
    "",              // 175
    "",              // 176
    "",              // 177
    "",              // 178
    "",              // 179
    "",              // 180
    "Numpad/",       // 181
    "",              // 182
    "PrtScr",        // 183
    "RAlt",          // 184
    "",              // 185
    "",              // 186
    "",              // 187
    "",              // 188
    "",              // 189
    "",              // 190
    "",              // 191
    "",              // 192
    "",              // 193
    "",              // 194
    "",              // 195
    "",              // 196
    "Pause",         // 197
    "",              // 198
    "Home",          // 199
    "Up",            // 200
    "PageUp",        // 201
    "",              // 202
    "Left",          // 203
    "",              // 204
    "Right",         // 205
    "",              // 206
    "End",           // 207
    "Down",          // 208
    "PageDown",      // 209
    "Insert",        // 210
    "Delete",        // 211
    "",              // 212
    "",              // 213
    "",              // 214
    "",              // 215
    "",              // 216
    "",              // 217
    "",              // 218
    "",              // 219
    "",              // 220
    "",              // 221
    "",              // 222
    "",              // 223
    "",              // 224
    "",              // 225
    "",              // 226
    "",              // 227
    "",              // 228
    "",              // 229
    "",              // 230
    "",              // 231
    "",              // 232
    "",              // 233
    "",              // 234
    "",              // 235
    "",              // 236
    "",              // 237
    "",              // 238
    "",              // 239
    "",              // 240
    "",              // 241
    "",              // 242
    "",              // 243
    "",              // 244
    "",              // 245
    "",              // 246
    "",              // 247
    "",              // 248
    "",              // 249
    "",              // 250
    "",              // 251
    "",              // 252
    "",              // 253
    "",              // 254
    "",              // 255
    "LMouse",        // 256
    "RMouse",        // 257
    "MMouse",        // 258
    "Mouse3",        // 259
    "Mouse4",        // 260
    "Mouse5",        // 261
    "Mouse6",        // 262
    "Mouse7",        // 263
    "MWheelUp",      // 264
    "MWheelDown",    // 265
    "DpadUp",        // 266
    "DpadDown",      // 267
    "DpadLeft",      // 268
    "DpadRight",     // 269
    "GamepadStart",  // 270
    "GamepadBack",   // 271
    "GamepadLS",     // 272
    "GamepadRS",     // 273
    "GamepadLB",     // 274
    "GamepadRB",     // 275
    "GamepadA",      // 276
    "GamepadB",      // 277
    "GamepadX",      // 278
    "GamepadY",      // 279
    "GamepadLT",     // 280
    "GamepadRT",     // 281
};

/// Bounds-checked accessor around `kKeycodeNames`. If input is invalid, returns the empty string.
constexpr std::string_view
KeycodeName(uint32_t keycode) {
    return keycode < kKeycodeNames.size() ? kKeycodeNames[keycode] : "";
}

constexpr bool
KeycodeIsValid(uint32_t keycode) {
    return !KeycodeName(keycode).empty();
}

/// If name is unknown, returns 0.
constexpr uint32_t
KeycodeFromName(std::string_view name) {
    for (uint32_t i = 0; i < kKeycodeNames.size(); i++) {
        if (name == kKeycodeNames[i]) {
            return i;
        }
    }
    return 0;
}

/// Normalizes invalid keycodes to 0. Valid keycodes are left as is.
constexpr uint32_t
KeycodeNormalized(uint32_t keycode) {
    return KeycodeIsValid(keycode) ? keycode : 0;
}

constexpr uint32_t
KeycodeFromScancode(uint32_t scancode, RE::INPUT_DEVICE device) {
    switch (device) {
        case RE::INPUT_DEVICE::kKeyboard:
            return scancode;
        case RE::INPUT_DEVICE::kMouse:
            return scancode + SKSE::InputMap::kMacro_MouseButtonOffset;
        case RE::INPUT_DEVICE::kGamepad:
            return SKSE::InputMap::GamepadMaskToKeycode(scancode);
    }
    return 0;
}

/// A button press action.
///
/// Invariants:
/// - `keycode_` is valid.
/// - `heldsecs_` nonnegative and finite.
/// - One of `IsKeyboard()`, `IsMouse()`, and `IsGamepad()` is true.
class Keystroke final {
  public:
    static void
    InputEventsToBuffer(const RE::InputEvent* events, std::vector<Keystroke>& buf) {
        for (; events; events = events->next) {
            auto keystroke = FromInputEvent(*events);
            if (keystroke) {
                buf.push_back(std::move(*keystroke));
            }
        }
    }

    static std::optional<Keystroke>
    FromInputEvent(const RE::InputEvent& event) {
        const auto* button = event.AsButtonEvent();
        if (!button || !button->HasIDCode() || !button->IsPressed()) {
            return std::nullopt;
        }
        auto keycode = KeycodeFromScancode(button->GetIDCode(), button->GetDevice());
        return New(keycode, button->HeldDuration());
    }

    static constexpr std::optional<Keystroke>
    New(uint32_t keycode, float heldsecs) {
        constexpr auto inf = std::numeric_limits<float>::infinity();
        if (KeycodeIsValid(keycode) && heldsecs >= 0.f && heldsecs < inf) {
            return Keystroke(keycode, heldsecs);
        }
        return std::nullopt;
    }

    constexpr uint32_t
    keycode() const {
        return keycode_;
    }

    constexpr float
    heldsecs() const {
        return heldsecs_;
    }

  private:
    explicit Keystroke(uint32_t keycode, float heldsecs) : keycode_(keycode), heldsecs_(heldsecs) {}

    uint32_t keycode_;
    float heldsecs_;
};

using Keyset = std::array<uint32_t, 4>;

/// Returns true if all keycodes are invalid.
constexpr bool
KeysetIsEmpty(const Keyset& keyset) {
    return !std::any_of(keyset.cbegin(), keyset.cend(), KeycodeIsValid);
}

/// Dedupes all valid keycodes, sorts all invalid keycodes to the back, and normalizes invalid
/// keycodes to 0.
constexpr Keyset
KeysetNormalized(Keyset keyset) {
    for (auto& keycode : keyset) {
        keycode = KeycodeNormalized(keycode);
    }

    std::sort(keyset.begin(), keyset.end(), [](uint32_t a, uint32_t b) {
        a = KeycodeIsValid(a) ? a : std::numeric_limits<uint32_t>::max();
        b = KeycodeIsValid(b) ? b : std::numeric_limits<uint32_t>::max();
        return a < b;
    });

    auto it = std::unique(keyset.begin(), keyset.end());
    for (; it != keyset.end(); it++) {
        *it = 0;
    }

    return keyset;
}

/// An ordered collection of 0 or more keysets.
///
/// Invariants:
/// - No keyset is empty.
/// - All keysets are normalized.
class Keysets final {
  public:
    enum class MatchResult {
        kNone,
        kPress,
        kSemihold,
        kHold,
    };

    /// Time (in seconds) a button must be pressed in order to be considered a "hold".
    static constexpr float kHoldThreshold = .5f;

    Keysets() = default;

    explicit Keysets(std::vector<Keyset> keysets) : keysets_(std::move(keysets)) {
        std::erase_if(keysets_, KeysetIsEmpty);
        for (auto& keyset : keysets_) {
            keyset = KeysetNormalized(keyset);
        }
    }

    const std::vector<Keyset>&
    vec() const {
        return keysets_;
    }

    /// Finds the first keyset matching `keystrokes`, then returns the nature of that
    /// match.
    MatchResult
    Match(std::span<const Keystroke> keystrokes) const {
        for (const auto& keyset : keysets_) {
            auto res = MatchOne(keyset, keystrokes);
            if (res != MatchResult::kNone) {
                return res;
            }
        }
        return MatchResult::kNone;
    }

  private:
    static MatchResult
    MatchOne(const Keyset& keyset, std::span<const Keystroke> keystrokes) {
        constexpr auto inf = std::numeric_limits<float>::infinity();
        auto min_heldsecs = inf;
        for (auto keycode : keyset) {
            if (!KeycodeIsValid(keycode)) {
                // keyset is sorted, no more valid keycodes to look at.
                break;
            }

            auto keystroke_is_matching = [=](const Keystroke& keystroke) {
                return keystroke.keycode() == keycode;
            };
            auto it = std::find_if(keystrokes.cbegin(), keystrokes.cend(), keystroke_is_matching);
            if (it == keystrokes.cend()) {
                // Current keyset keycode does not match any keystroke.
                return MatchResult::kNone;
            }
            if (it->heldsecs() < min_heldsecs) {
                min_heldsecs = it->heldsecs();
            }
        }

        if (min_heldsecs == inf) {
            return MatchResult::kNone;
        } else if (min_heldsecs >= kHoldThreshold) {
            return MatchResult::kHold;
        } else if (min_heldsecs > .0f) {
            return MatchResult::kSemihold;
        } else {
            return MatchResult::kPress;
        }
    }

    std::vector<Keyset> keysets_;
};

}  // namespace ech
