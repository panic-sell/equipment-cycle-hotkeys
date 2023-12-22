/// UI-related function hooks and input handlers.
#pragma once

#include "dev_util.h"
#include "fs.h"
#include "keys.h"
#include "serde.h"
#include "settings.h"
#include "ui_drawing.h"
#include "ui_viewmodels.h"

namespace ech {
namespace ui {
namespace internal {

/// This only contains keys that should be fed into `io.AddKeyEvent()`. I.e. Mouse keys are not
/// mapped.
/// Whether the UI is visible. Equivalently, whether the UI is capturing all input.
inline std::atomic<bool> gIsActive = false;

constexpr ImGuiKey
ImGuiKeyFromKeycode(uint32_t keycode) {
    constexpr auto keys = std::array{
        ImGuiKey_None,              // 0
        ImGuiKey_Escape,            // 1
        ImGuiKey_1,                 // 2
        ImGuiKey_2,                 // 3
        ImGuiKey_3,                 // 4
        ImGuiKey_4,                 // 5
        ImGuiKey_5,                 // 6
        ImGuiKey_6,                 // 7
        ImGuiKey_7,                 // 8
        ImGuiKey_8,                 // 9
        ImGuiKey_9,                 // 10
        ImGuiKey_0,                 // 11
        ImGuiKey_Minus,             // 12
        ImGuiKey_Equal,             // 13
        ImGuiKey_Backspace,         // 14
        ImGuiKey_Tab,               // 15
        ImGuiKey_Q,                 // 16
        ImGuiKey_W,                 // 17
        ImGuiKey_E,                 // 18
        ImGuiKey_R,                 // 19
        ImGuiKey_T,                 // 20
        ImGuiKey_Y,                 // 21
        ImGuiKey_U,                 // 22
        ImGuiKey_I,                 // 23
        ImGuiKey_O,                 // 24
        ImGuiKey_P,                 // 25
        ImGuiKey_LeftBracket,       // 26
        ImGuiKey_RightBracket,      // 27
        ImGuiKey_Enter,             // 28
        ImGuiKey_LeftCtrl,          // 29
        ImGuiKey_A,                 // 30
        ImGuiKey_S,                 // 31
        ImGuiKey_D,                 // 32
        ImGuiKey_F,                 // 33
        ImGuiKey_G,                 // 34
        ImGuiKey_H,                 // 35
        ImGuiKey_J,                 // 36
        ImGuiKey_K,                 // 37
        ImGuiKey_L,                 // 38
        ImGuiKey_Semicolon,         // 39
        ImGuiKey_Apostrophe,        // 40
        ImGuiKey_GraveAccent,       // 41
        ImGuiKey_LeftShift,         // 42
        ImGuiKey_Backslash,         // 43
        ImGuiKey_Z,                 // 44
        ImGuiKey_X,                 // 45
        ImGuiKey_C,                 // 46
        ImGuiKey_V,                 // 47
        ImGuiKey_B,                 // 48
        ImGuiKey_N,                 // 49
        ImGuiKey_M,                 // 50
        ImGuiKey_Comma,             // 51
        ImGuiKey_Period,            // 52
        ImGuiKey_Slash,             // 53
        ImGuiKey_RightShift,        // 54
        ImGuiKey_KeypadMultiply,    // 55
        ImGuiKey_LeftAlt,           // 56
        ImGuiKey_Space,             // 57
        ImGuiKey_CapsLock,          // 58
        ImGuiKey_F1,                // 59
        ImGuiKey_F2,                // 60
        ImGuiKey_F3,                // 61
        ImGuiKey_F4,                // 62
        ImGuiKey_F5,                // 63
        ImGuiKey_F6,                // 64
        ImGuiKey_F7,                // 65
        ImGuiKey_F8,                // 66
        ImGuiKey_F9,                // 67
        ImGuiKey_F10,               // 68
        ImGuiKey_NumLock,           // 69
        ImGuiKey_ScrollLock,        // 70
        ImGuiKey_Keypad7,           // 71
        ImGuiKey_Keypad8,           // 72
        ImGuiKey_Keypad9,           // 73
        ImGuiKey_KeypadSubtract,    // 74
        ImGuiKey_Keypad4,           // 75
        ImGuiKey_Keypad5,           // 76
        ImGuiKey_Keypad6,           // 77
        ImGuiKey_KeypadAdd,         // 78
        ImGuiKey_Keypad1,           // 79
        ImGuiKey_Keypad2,           // 80
        ImGuiKey_Keypad3,           // 81
        ImGuiKey_Keypad0,           // 82
        ImGuiKey_KeypadDecimal,     // 83
        ImGuiKey_None,              // 84
        ImGuiKey_None,              // 85
        ImGuiKey_None,              // 86
        ImGuiKey_F11,               // 87
        ImGuiKey_F12,               // 88
        ImGuiKey_None,              // 89
        ImGuiKey_None,              // 90
        ImGuiKey_None,              // 91
        ImGuiKey_None,              // 92
        ImGuiKey_None,              // 93
        ImGuiKey_None,              // 94
        ImGuiKey_None,              // 95
        ImGuiKey_None,              // 96
        ImGuiKey_None,              // 97
        ImGuiKey_None,              // 98
        ImGuiKey_None,              // 99
        ImGuiKey_None,              // 100
        ImGuiKey_None,              // 101
        ImGuiKey_None,              // 102
        ImGuiKey_None,              // 103
        ImGuiKey_None,              // 104
        ImGuiKey_None,              // 105
        ImGuiKey_None,              // 106
        ImGuiKey_None,              // 107
        ImGuiKey_None,              // 108
        ImGuiKey_None,              // 109
        ImGuiKey_None,              // 110
        ImGuiKey_None,              // 111
        ImGuiKey_None,              // 112
        ImGuiKey_None,              // 113
        ImGuiKey_None,              // 114
        ImGuiKey_None,              // 115
        ImGuiKey_None,              // 116
        ImGuiKey_None,              // 117
        ImGuiKey_None,              // 118
        ImGuiKey_None,              // 119
        ImGuiKey_None,              // 120
        ImGuiKey_None,              // 121
        ImGuiKey_None,              // 122
        ImGuiKey_None,              // 123
        ImGuiKey_None,              // 124
        ImGuiKey_None,              // 125
        ImGuiKey_None,              // 126
        ImGuiKey_None,              // 127
        ImGuiKey_None,              // 128
        ImGuiKey_None,              // 129
        ImGuiKey_None,              // 130
        ImGuiKey_None,              // 131
        ImGuiKey_None,              // 132
        ImGuiKey_None,              // 133
        ImGuiKey_None,              // 134
        ImGuiKey_None,              // 135
        ImGuiKey_None,              // 136
        ImGuiKey_None,              // 137
        ImGuiKey_None,              // 138
        ImGuiKey_None,              // 139
        ImGuiKey_None,              // 140
        ImGuiKey_None,              // 141
        ImGuiKey_None,              // 142
        ImGuiKey_None,              // 143
        ImGuiKey_None,              // 144
        ImGuiKey_None,              // 145
        ImGuiKey_None,              // 146
        ImGuiKey_None,              // 147
        ImGuiKey_None,              // 148
        ImGuiKey_None,              // 149
        ImGuiKey_None,              // 150
        ImGuiKey_None,              // 151
        ImGuiKey_None,              // 152
        ImGuiKey_None,              // 153
        ImGuiKey_None,              // 154
        ImGuiKey_None,              // 155
        ImGuiKey_KeypadEnter,       // 156
        ImGuiKey_RightCtrl,         // 157
        ImGuiKey_None,              // 158
        ImGuiKey_None,              // 159
        ImGuiKey_None,              // 160
        ImGuiKey_None,              // 161
        ImGuiKey_None,              // 162
        ImGuiKey_None,              // 163
        ImGuiKey_None,              // 164
        ImGuiKey_None,              // 165
        ImGuiKey_None,              // 166
        ImGuiKey_None,              // 167
        ImGuiKey_None,              // 168
        ImGuiKey_None,              // 169
        ImGuiKey_None,              // 170
        ImGuiKey_None,              // 171
        ImGuiKey_None,              // 172
        ImGuiKey_None,              // 173
        ImGuiKey_None,              // 174
        ImGuiKey_None,              // 175
        ImGuiKey_None,              // 176
        ImGuiKey_None,              // 177
        ImGuiKey_None,              // 178
        ImGuiKey_None,              // 179
        ImGuiKey_None,              // 180
        ImGuiKey_KeypadDivide,      // 181
        ImGuiKey_None,              // 182
        ImGuiKey_PrintScreen,       // 183
        ImGuiKey_RightAlt,          // 184
        ImGuiKey_None,              // 185
        ImGuiKey_None,              // 186
        ImGuiKey_None,              // 187
        ImGuiKey_None,              // 188
        ImGuiKey_None,              // 189
        ImGuiKey_None,              // 190
        ImGuiKey_None,              // 191
        ImGuiKey_None,              // 192
        ImGuiKey_None,              // 193
        ImGuiKey_None,              // 194
        ImGuiKey_None,              // 195
        ImGuiKey_None,              // 196
        ImGuiKey_Pause,             // 197
        ImGuiKey_None,              // 198
        ImGuiKey_Home,              // 199
        ImGuiKey_UpArrow,           // 200
        ImGuiKey_PageUp,            // 201
        ImGuiKey_None,              // 202
        ImGuiKey_LeftArrow,         // 203
        ImGuiKey_None,              // 204
        ImGuiKey_RightArrow,        // 205
        ImGuiKey_None,              // 206
        ImGuiKey_End,               // 207
        ImGuiKey_DownArrow,         // 208
        ImGuiKey_PageDown,          // 209
        ImGuiKey_Insert,            // 210
        ImGuiKey_Delete,            // 211
        ImGuiKey_None,              // 212
        ImGuiKey_None,              // 213
        ImGuiKey_None,              // 214
        ImGuiKey_None,              // 215
        ImGuiKey_None,              // 216
        ImGuiKey_None,              // 217
        ImGuiKey_None,              // 218
        ImGuiKey_None,              // 219
        ImGuiKey_None,              // 220
        ImGuiKey_None,              // 221
        ImGuiKey_None,              // 222
        ImGuiKey_None,              // 223
        ImGuiKey_None,              // 224
        ImGuiKey_None,              // 225
        ImGuiKey_None,              // 226
        ImGuiKey_None,              // 227
        ImGuiKey_None,              // 228
        ImGuiKey_None,              // 229
        ImGuiKey_None,              // 230
        ImGuiKey_None,              // 231
        ImGuiKey_None,              // 232
        ImGuiKey_None,              // 233
        ImGuiKey_None,              // 234
        ImGuiKey_None,              // 235
        ImGuiKey_None,              // 236
        ImGuiKey_None,              // 237
        ImGuiKey_None,              // 238
        ImGuiKey_None,              // 239
        ImGuiKey_None,              // 240
        ImGuiKey_None,              // 241
        ImGuiKey_None,              // 242
        ImGuiKey_None,              // 243
        ImGuiKey_None,              // 244
        ImGuiKey_None,              // 245
        ImGuiKey_None,              // 246
        ImGuiKey_None,              // 247
        ImGuiKey_None,              // 248
        ImGuiKey_None,              // 249
        ImGuiKey_None,              // 250
        ImGuiKey_None,              // 251
        ImGuiKey_None,              // 252
        ImGuiKey_None,              // 253
        ImGuiKey_None,              // 254
        ImGuiKey_None,              // 255
        ImGuiKey_None,              // 256
        ImGuiKey_None,              // 257
        ImGuiKey_None,              // 258
        ImGuiKey_None,              // 259
        ImGuiKey_None,              // 260
        ImGuiKey_None,              // 261
        ImGuiKey_None,              // 262
        ImGuiKey_None,              // 263
        ImGuiKey_None,              // 264
        ImGuiKey_None,              // 265
        ImGuiKey_GamepadDpadUp,     // 266
        ImGuiKey_GamepadDpadDown,   // 267
        ImGuiKey_GamepadDpadLeft,   // 268
        ImGuiKey_GamepadDpadRight,  // 269
        ImGuiKey_GamepadStart,      // 270
        ImGuiKey_GamepadBack,       // 271
        ImGuiKey_GamepadL3,         // 272
        ImGuiKey_GamepadR3,         // 273
        ImGuiKey_GamepadL1,         // 274
        ImGuiKey_GamepadR1,         // 275
        ImGuiKey_GamepadFaceDown,   // 276
        ImGuiKey_GamepadFaceRight,  // 277
        ImGuiKey_GamepadFaceLeft,   // 278
        ImGuiKey_GamepadFaceUp,     // 279
        ImGuiKey_GamepadL2,         // 280
        ImGuiKey_GamepadR2,         // 281
    };

    return keycode < keys.size() ? keys[keycode] : ImGuiKey_None;
}

inline bool
CaptureKeyboardInput(const RE::ButtonEvent& button) {
    if (button.GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
        return false;
    }

    auto& io = ImGui::GetIO();
    auto scancode = button.GetIDCode();
    auto is_pressed = button.IsPressed();
    auto imgui_key = ImGuiKeyFromKeycode(scancode);
    if (imgui_key == ImGuiKey_None) {
        return true;
    }

    io.AddKeyEvent(imgui_key, is_pressed);
    switch (imgui_key) {
        case ImGuiKey_LeftCtrl:
        case ImGuiKey_RightCtrl:
            io.AddKeyEvent(ImGuiMod_Ctrl, is_pressed);
            break;
        case ImGuiKey_LeftShift:
        case ImGuiKey_RightShift:
            io.AddKeyEvent(ImGuiMod_Shift, is_pressed);
            break;
        case ImGuiKey_LeftAlt:
        case ImGuiKey_RightAlt:
            io.AddKeyEvent(ImGuiMod_Alt, is_pressed);
            break;
    }

    // Fetch a character only once per key press.
    if (!button.IsDown()) {
        return true;
    }

    auto vk_keystate = std::array<uint8_t, 256>{0};
    {
        auto* device_man = RE::BSInputDeviceManager::GetSingleton();
        auto* sc_keyboard = device_man ? device_man->GetKeyboard() : nullptr;
        auto* sc_keystate = sc_keyboard ? sc_keyboard->curState : nullptr;
        if (!sc_keyboard || !sc_keystate) {
            return true;
        }
        for (UINT sc = 0; sc < vk_keystate.size(); sc++) {
            auto vk = MapVirtualKeyW(sc, MAPVK_VSC_TO_VK);
            if (vk >= 256) {
                continue;
            }
            vk_keystate[vk] |= sc_keystate[sc];
            if (vk == VK_CAPITAL && sc_keyboard->capsLockOn) {
                vk_keystate[vk] |= 1;
            }
        }
    }

    auto vk = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK);
    auto buf = std::array<WCHAR, 4>{0};
    auto count = ToUnicode(vk, scancode, &vk_keystate[0], &buf[0], static_cast<int>(buf.size()), 0);
    for (int i = 0; i < count; i++) {
        io.AddInputCharacterUTF16(buf[i]);
    }
    return true;
}

inline bool
CaptureMouseInput(const RE::ButtonEvent& button) {
    if (button.GetDevice() != RE::INPUT_DEVICE::kMouse) {
        return false;
    }

    auto& io = ImGui::GetIO();
    auto scancode = button.GetIDCode();
    auto is_pressed = button.IsPressed();

    if (scancode < ImGuiMouseButton_COUNT) {  // Ignore keycodes 261-263.
        io.AddMouseButtonEvent(scancode, is_pressed);
    } else if (scancode == 8) {  // Keycode 264
        io.AddMouseWheelEvent(0, 1);
    } else if (scancode == 9) {  // Keycode 265
        io.AddMouseWheelEvent(0, -1);
    }
    return true;
}

inline bool
CaptureGamepadInput(const RE::ButtonEvent& button) {
    if (button.GetDevice() != RE::INPUT_DEVICE::kGamepad) {
        return false;
    }
    // TODO
    return true;
}

/// Forwards inputs to ImGui.
inline void
CaptureInputs(const RE::InputEvent* events) {
    for (; events; events = events->next) {
        const auto* button = events->AsButtonEvent();
        if (!button) {
            continue;
        }
        CaptureMouseInput(*button) || CaptureKeyboardInput(*button) || CaptureGamepadInput(*button);
    }
}

/// Checks if UI toggle keys were pressed, and activates/deactivates the UI accordingly.
inline void
MaybeToggleActiveState(const RE::InputEvent* events) {
    static std::vector<Keystroke> keybuf;
    keybuf.clear();
    Keystroke::InputEventsToBuffer(events, keybuf);
    if (Settings::GetSingleton().menu_toggle_keysets.Match(keybuf)
        != Keysets::MatchResult::kPress) {
        return;
    }
    auto& io = ImGui::GetIO();
    if (gIsActive.load()) {
        // Immediately disable state, then perform cleanup.
        gIsActive.store(false);
        io.MouseDrawCursor = false;
    } else {
        // Set things up before enabling state.
        io.MouseDrawCursor = true;
        gIsActive.store(true);
    }
}

inline REL::Relocation<void(RE::BSTEventSource<RE::InputEvent*>*, RE::InputEvent* const*)>
    HandleInput;

inline void
HandleInputHook(RE::BSTEventSource<RE::InputEvent*>* event_src, RE::InputEvent* const* events) {
    if (!events) {
        HandleInput(event_src, events);
        return;
    }

    MaybeToggleActiveState(*events);
    if (!gIsActive.load()) {
        HandleInput(event_src, events);
        return;
    }

    CaptureInputs(*events);
    auto dummy = std::array{static_cast<RE::InputEvent*>(nullptr)};
    HandleInput(event_src, &dummy[0]);
}

inline REL::Relocation<void(uint32_t)> Render;

inline void
RenderHook(uint32_t x) {
    Render(x);
    if (!gIsActive.load()) {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Draw();
    // ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

inline void
Configure() {
    auto& io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = fs::kImGuiIniPath;

    const auto& settings = Settings::GetSingleton();

    io.FontGlobalScale = settings.font_scale;
    switch (settings.color_style) {
        case 0:
            ImGui::StyleColorsDark();
            break;
        case 1:
            ImGui::StyleColorsLight();
            break;
        case 2:
            ImGui::StyleColorsClassic();
            break;
        default:
            ImGui::StyleColorsDark();
            break;
    }
}

}  // namespace internal

/// Must complete before `InstallHooks()`.
[[nodiscard]] inline std::expected<void, std::string_view>
Init() {
    auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
    auto* device = renderer ? renderer->data.forwarder : nullptr;
    auto* ctx = renderer ? renderer->data.context : nullptr;
    auto* swapchain = renderer ? renderer->data.renderWindows[0].swapChain : nullptr;
    if (!device || !ctx || !swapchain) {
        return std::unexpected("failed to get renderer");
    }

    DXGI_SWAP_CHAIN_DESC sd;
    if (swapchain->GetDesc(&sd) != S_OK) {
        return std::unexpected("failed to get swap chain description");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    internal::Configure();

    if (!ImGui_ImplWin32_Init(sd.OutputWindow) || !ImGui_ImplDX11_Init(device, ctx)) {
        return std::unexpected("failed to initialize Dear ImGui components");
    }

    SKSE::log::info("UI initialized");
    return {};
}

inline void
InstallHooks() {
    auto& trampoline = SKSE::GetTrampoline();
    trampoline.create(28);

    auto loc = REL::Relocation<uintptr_t>(REL::RelocationID(75461, 77246), REL::Offset(0x9));
    internal::Render = trampoline.write_call<5>(loc.address(), internal::RenderHook);

    loc = REL::Relocation<uintptr_t>(REL::RelocationID(67315, 68617), REL::Offset(0x7b));
    internal::HandleInput = trampoline.write_call<5>(loc.address(), internal::HandleInputHook);
}

}  // namespace ui
}  // namespace ech
