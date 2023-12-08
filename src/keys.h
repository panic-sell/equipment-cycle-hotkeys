#pragma once

namespace ech {
namespace internal {

// https://ck.uesp.net/wiki/Input_Script
constexpr inline auto kKeycodeNames = std::array{
    ""sv,               // 0
    "esc"sv,            // 1
    "1"sv,              // 2
    "2"sv,              // 3
    "3"sv,              // 4
    "4"sv,              // 5
    "5"sv,              // 6
    "6"sv,              // 7
    "7"sv,              // 8
    "8"sv,              // 9
    "9"sv,              // 10
    "0"sv,              // 11
    "-"sv,              // 12
    "="sv,              // 13
    "backspace"sv,      // 14
    "tab"sv,            // 15
    "q"sv,              // 16
    "w"sv,              // 17
    "e"sv,              // 18
    "r"sv,              // 19
    "t"sv,              // 20
    "y"sv,              // 21
    "u"sv,              // 22
    "i"sv,              // 23
    "o"sv,              // 24
    "p"sv,              // 25
    "["sv,              // 26
    "]"sv,              // 27
    "enter"sv,          // 28
    "lctrl"sv,          // 29
    "a"sv,              // 30
    "s"sv,              // 31
    "d"sv,              // 32
    "f"sv,              // 33
    "g"sv,              // 34
    "h"sv,              // 35
    "j"sv,              // 36
    "k"sv,              // 37
    "l"sv,              // 38
    ";"sv,              // 39
    "'"sv,              // 40
    "`"sv,              // 41
    "lshift"sv,         // 42
    "\\"sv,             // 43
    "z"sv,              // 44
    "x"sv,              // 45
    "c"sv,              // 46
    "v"sv,              // 47
    "b"sv,              // 48
    "n"sv,              // 49
    "m"sv,              // 50
    ","sv,              // 51
    "."sv,              // 52
    "/"sv,              // 53
    "rshift"sv,         // 54
    "numpad*"sv,        // 55
    "lalt"sv,           // 56
    "space"sv,          // 57
    "capslock"sv,       // 58
    "f1"sv,             // 59
    "f2"sv,             // 60
    "f3"sv,             // 61
    "f4"sv,             // 62
    "f5"sv,             // 63
    "f6"sv,             // 64
    "f7"sv,             // 65
    "f8"sv,             // 66
    "f9"sv,             // 67
    "f10"sv,            // 68
    "numlock"sv,        // 69
    "scrolllock"sv,     // 70
    "numpad7"sv,        // 71
    "numpad8"sv,        // 72
    "numpad9"sv,        // 73
    "numpad-"sv,        // 74
    "numpad4"sv,        // 75
    "numpad5"sv,        // 76
    "numpad6"sv,        // 77
    "numpad+"sv,        // 78
    "numpad1"sv,        // 79
    "numpad2"sv,        // 80
    "numpad3"sv,        // 81
    "numpad0"sv,        // 82
    "numpad."sv,        // 83
    ""sv,               // 84
    ""sv,               // 85
    ""sv,               // 86
    "f11"sv,            // 87
    "f12"sv,            // 88
    ""sv,               // 89
    ""sv,               // 90
    ""sv,               // 91
    ""sv,               // 92
    ""sv,               // 93
    ""sv,               // 94
    ""sv,               // 95
    ""sv,               // 96
    ""sv,               // 97
    ""sv,               // 98
    ""sv,               // 99
    ""sv,               // 100
    ""sv,               // 101
    ""sv,               // 102
    ""sv,               // 103
    ""sv,               // 104
    ""sv,               // 105
    ""sv,               // 106
    ""sv,               // 107
    ""sv,               // 108
    ""sv,               // 109
    ""sv,               // 110
    ""sv,               // 111
    ""sv,               // 112
    ""sv,               // 113
    ""sv,               // 114
    ""sv,               // 115
    ""sv,               // 116
    ""sv,               // 117
    ""sv,               // 118
    ""sv,               // 119
    ""sv,               // 120
    ""sv,               // 121
    ""sv,               // 122
    ""sv,               // 123
    ""sv,               // 124
    ""sv,               // 125
    ""sv,               // 126
    ""sv,               // 127
    ""sv,               // 128
    ""sv,               // 129
    ""sv,               // 130
    ""sv,               // 131
    ""sv,               // 132
    ""sv,               // 133
    ""sv,               // 134
    ""sv,               // 135
    ""sv,               // 136
    ""sv,               // 137
    ""sv,               // 138
    ""sv,               // 139
    ""sv,               // 140
    ""sv,               // 141
    ""sv,               // 142
    ""sv,               // 143
    ""sv,               // 144
    ""sv,               // 145
    ""sv,               // 146
    ""sv,               // 147
    ""sv,               // 148
    ""sv,               // 149
    ""sv,               // 150
    ""sv,               // 151
    ""sv,               // 152
    ""sv,               // 153
    ""sv,               // 154
    ""sv,               // 155
    "numpadenter"sv,    // 156
    "rctrl"sv,          // 157
    ""sv,               // 158
    ""sv,               // 159
    ""sv,               // 160
    ""sv,               // 161
    ""sv,               // 162
    ""sv,               // 163
    ""sv,               // 164
    ""sv,               // 165
    ""sv,               // 166
    ""sv,               // 167
    ""sv,               // 168
    ""sv,               // 169
    ""sv,               // 170
    ""sv,               // 171
    ""sv,               // 172
    ""sv,               // 173
    ""sv,               // 174
    ""sv,               // 175
    ""sv,               // 176
    ""sv,               // 177
    ""sv,               // 178
    ""sv,               // 179
    ""sv,               // 180
    "numpad/"sv,        // 181
    ""sv,               // 182
    "prtsc"sv,          // 183
    "ralt"sv,           // 184
    ""sv,               // 185
    ""sv,               // 186
    ""sv,               // 187
    ""sv,               // 188
    ""sv,               // 189
    ""sv,               // 190
    ""sv,               // 191
    ""sv,               // 192
    ""sv,               // 193
    ""sv,               // 194
    ""sv,               // 195
    ""sv,               // 196
    "pause"sv,          // 197
    ""sv,               // 198
    "home"sv,           // 199
    "up"sv,             // 200
    "pgup"sv,           // 201
    ""sv,               // 202
    "left"sv,           // 203
    ""sv,               // 204
    "right"sv,          // 205
    ""sv,               // 206
    "end"sv,            // 207
    "down"sv,           // 208
    "pgdown"sv,         // 209
    "insert"sv,         // 210
    "delete"sv,         // 211
    ""sv,               // 212
    ""sv,               // 213
    ""sv,               // 214
    ""sv,               // 215
    ""sv,               // 216
    ""sv,               // 217
    ""sv,               // 218
    ""sv,               // 219
    ""sv,               // 220
    ""sv,               // 221
    ""sv,               // 222
    ""sv,               // 223
    ""sv,               // 224
    ""sv,               // 225
    ""sv,               // 226
    ""sv,               // 227
    ""sv,               // 228
    ""sv,               // 229
    ""sv,               // 230
    ""sv,               // 231
    ""sv,               // 232
    ""sv,               // 233
    ""sv,               // 234
    ""sv,               // 235
    ""sv,               // 236
    ""sv,               // 237
    ""sv,               // 238
    ""sv,               // 239
    ""sv,               // 240
    ""sv,               // 241
    ""sv,               // 242
    ""sv,               // 243
    ""sv,               // 244
    ""sv,               // 245
    ""sv,               // 246
    ""sv,               // 247
    ""sv,               // 248
    ""sv,               // 249
    ""sv,               // 250
    ""sv,               // 251
    ""sv,               // 252
    ""sv,               // 253
    ""sv,               // 254
    ""sv,               // 255
    "lmouse"sv,         // 256
    "rmouse"sv,         // 257
    "mmouse"sv,         // 258
    "mouse3"sv,         // 259
    "mouse4"sv,         // 260
    "mouse5"sv,         // 261
    "mouse6"sv,         // 262
    "mouse7"sv,         // 263
    "mwheelup"sv,       // 264
    "mwheeldown"sv,     // 265
    "dpad_up"sv,        // 266
    "dpad_down"sv,      // 267
    "dpad_left"sv,      // 268
    "dpad_right"sv,     // 269
    "gamepad_start"sv,  // 270
    "gamepad_back"sv,   // 271
    "gamepad_ls"sv,     // 272
    "gamepad_rs"sv,     // 273
    "gamepad_lb"sv,     // 274
    "gamepad_rb"sv,     // 275
    "gamepad_a"sv,      // 276
    "gamepad_b"sv,      // 277
    "gamepad_x"sv,      // 278
    "gamepad_y"sv,      // 279
    "gamepad_lt"sv,     // 280
    "gamepad_rt"sv,     // 281
};

}  // namespace internal

/// If input is invalid, returns some value X for which `KeycodeIsValid(KeycodeFromName(X)) ==
/// false`.
constexpr std::string_view
KeycodeName(uint32_t keycode) {
    return keycode < internal::kKeycodeNames.size() ? internal::kKeycodeNames[keycode] : ""sv;
}

constexpr bool
KeycodeIsValid(uint32_t keycode) {
    return KeycodeName(keycode) != ""sv;
}

/// If name is unknown, returns some value X for which `KeycodeIsValid(X) == false`.
constexpr uint32_t
KeycodeFromName(std::string_view name) {
    for (uint32_t i = 0; i < internal::kKeycodeNames.size(); i++) {
        if (name == internal::kKeycodeNames[i]) {
            return i;
        }
    }
    return 0;
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
class Keystroke {
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
        auto* button = event.AsButtonEvent();
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
    Keycode() const {
        return keycode_;
    }

    constexpr float
    Heldsecs() const {
        return heldsecs_;
    }

  private:
    explicit Keystroke(uint32_t keycode, float heldsecs) : keycode_(keycode), heldsecs_(heldsecs) {}

    uint32_t keycode_;
    float heldsecs_;
};

using Keyset = std::array<uint32_t, 4>;

/// An ordered collection of 0 or more keysets.
///
/// Invariants:
/// - All keysets have at least one valid keycode.
/// - All keysets are sorted such that valid keycodes precede invalid ones.
class Keysets {
  public:
    enum class MatchResult {
        kNone,
        kPress,
        kSemihold,
        kHold,
    };

    /// Time (in seconds) a button must be pressed in order to be considered a "hold".
    static constexpr inline float kHoldThreshold = .5f;

    Keysets() = default;

    explicit Keysets(std::vector<Keyset> keysets) : keysets_(std::move(keysets)) {
        for (auto& keyset : keysets_) {
            // Normalize invalid keycodes.
            for (auto& keycode : keyset) {
                if (!KeycodeIsValid(keycode)) {
                    keycode = 0;
                }
            }

            std::sort(keyset.begin(), keyset.end(), [](uint32_t a, uint32_t b) {
                a = KeycodeIsValid(a) ? a : std::numeric_limits<uint32_t>::max();
                b = KeycodeIsValid(b) ? b : std::numeric_limits<uint32_t>::max();
                return a < b;
            });

            // Remove duplicates.
            size_t i = 0;
            for (size_t j = 1; j < keyset.size(); j++) {
                if (keyset[i] != keyset[j]) {
                    i++;
                    keyset[i] = keyset[j];
                }
            }
            for (i++; i < keyset.size(); i++) {
                keyset[i] = 0;
            }
        }

        auto keyset_is_empty = [](const Keyset& keyset) {
            for (auto keycode : keyset) {
                if (KeycodeIsValid(keycode)) {
                    return false;
                }
            }
            return true;
        };
        std::erase_if(keysets_, keyset_is_empty);
    }

    const std::vector<Keyset>&
    Vec() const {
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
                return keystroke.Keycode() == keycode;
            };
            auto it = std::find_if(keystrokes.cbegin(), keystrokes.cend(), keystroke_is_matching);
            if (it == keystrokes.cend()) {
                // Current keyset keycode does not match any keystroke.
                return MatchResult::kNone;
            }
            if (it->Heldsecs() < min_heldsecs) {
                min_heldsecs = it->Heldsecs();
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
