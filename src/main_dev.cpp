// Program for exploring Dear ImGui.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <d3d11.h>
#include <imgui_internal.h>
#include <tchar.h>

#define ECH_UI_DEV
#include "fs.h"
#include "keys.h"
#include "serde.h"
#include "ui_drawing.h"
#include "ui_plumbing.h"
#include "ui_viewmodels.h"

using namespace ech;

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#if 0
namespace json = boost::json;

namespace custom_ns {

// test nan, test int narrowing
template <typename T>
struct A {
    int8_t i8 = 127;
    float f32 = 1.f;
    std::vector<std::string_view> sv = {"abc", "def"};
    T t;
};

template <typename T>
inline void
tag_invoke(const json::value_from_tag&, json::value& jv, const A<T>& a) {
    json::object o;
    o.insert_or_assign("i8", a.i8);
    o.insert_or_assign("f32", a.f32);
    jv = o;
}

template <typename T>
inline json::result_for<A<T>, json::value>::type
tag_invoke(const json::try_value_to_tag<A<T>>&, const json::value& jv) {
    if (!jv.is_object()) {
        return json::make_error_code(json::error::not_object);
    }
    A<T> a;
    const auto& o = jv.as_object();
    if (auto it = o.find("i8"); it != o.end()) {
        const auto& [__, v] = *it;
        if (v.is_double()) {
            a.i8 = static_cast<decltype(a.i8)>(v.as_double());
        } else if (v.is_int64()) {
            a.i8 = static_cast<decltype(a.i8)>(v.as_int64());
        } else if (v.is_uint64()) {
            a.i8 = static_cast<decltype(a.i8)>(v.as_uint64());
        }
    }
    if (auto it = o.find("f32"); it != o.end()) {
        const auto& [__, v] = *it;
        if (v.is_double()) {
            a.f32 = static_cast<decltype(a.f32)>(v.as_double());
        } else if (v.is_int64()) {
            a.f32 = static_cast<decltype(a.f32)>(v.as_int64());
        } else if (v.is_uint64()) {
            a.f32 = static_cast<decltype(a.f32)>(v.as_uint64());
        }
    }
    return a;
}

}  // namespace custom_ns

int
main() {
    using namespace ech;

    json::parse_options opts;
    opts.numbers = json::number_precision::precise;
    opts.allow_comments = true;
    opts.allow_trailing_commas = true;

    // R"({
    //         "name": "Hotkey",
    //         "keysets": [["esc", "", "gamepADRt", "", "TAB"]],
    //         "equipsets": [2, 3, 4],
    //         "active_equipset": 1
    //     })"

    json::value j;
    json::error_code ec;
    j = json::parse(
        R"({
                "active_hotkey": 1,
                "hotkeys": [
                    {
                        "name": "hk0",
                        "keysets": [
                            ["0"]
                        ],
                        "active_equipset": 0,
                        "equipsets": [
                            [
                                {"slot": 0, "unequip": true},
                                {"slot": 1, "unequip": true},
                                {"slot": 2, "unequip": true},
                                {"slot": 3, "unequip": true},
                            ],
                        ],
                    },
                    {
                        "name": "hk1",
                        "keysets": [
                            ["LShift", "1"],
                            ["RShift", "1"],
                        ],
                        "active_equipset": 1,
                        "equipsets": [
                            [
                                {"slot": 0, "unequip": true},
                                {"slot": 1, "unequip": true},
                            ],
                            [
                                {"slot": 2, "unequip": true},
                                {"slot": 3, "unequip": true},
                            ],
                        ],
                    },
                ],
            })",
        ec,
        {},
        opts
    );
    if (ec) {
        std::println("{}", ec.what());
        return 1;
    }
    std::println("{}\n", json::serialize(j));
    auto k = json::try_value_to<Hotkeys<>>(j, SerdeContext());
    if (!k) {
        std::println("{}", k.error().what());
        return 1;
    }
    auto j_final = json::value_from(*k, SerdeContext());
    std::println("{}", json::serialize(j_final));
}
#else
// Main code
int
main(int, char**) {
    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandle(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"ImGui Example",
        nullptr
    };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"Dear ImGui DirectX11 Example",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        1280,
        800,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto settings = fs::Read(fs::kSettingsPath).and_then([](std::string&& s) {
        return Deserialize<Settings>(s);
    });
    ui::internal::Configure(settings ? *settings : Settings());

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple
    // fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the
    // font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those
    // errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture
    // when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below
    // will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher
    // quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to
    // write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
    // nullptr, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static auto ctx = []() {
            auto c = ui::Context{.hotkeys_ui{
                {
                    .name = "asdf",
                    .keysets{
                        {1, 2, 3, 4},
                        {5, 0, 45, 104},
                        {7, 0, 0, 0},
                        {4, 3, 2, 1},
                        {0, 20, 19, 18},
                        {0},
                    },
                    .equipsets{
                        {},
                        {},
                        {},
                        {},
                    },
                },
            }};
            c.ReloadProfileCache();
            return c;
        }();
        ui::Draw(ctx);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()!
        // You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin(
                "Another Window", &show_another_window
            );  // Pass a pointer to our bool variable (the window will have a closing button
                // that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) {
                show_another_window = false;
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);  // Present with vsync
        // g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}
#endif

// Helper functions

bool
CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );
    if (res == DXGI_ERROR_UNSUPPORTED) {  // Try high-performance WARP software driver if
                                          // hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext
        );
    }
    if (res != S_OK) {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void
CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void
CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void
CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui
// wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
// application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
// application, or clear/overwrite your copy of the keyboard data. Generally you may always pass
// all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI
WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                return 0;
            }
            g_ResizeWidth = (UINT)LOWORD(lParam);  // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) {  // Disable ALT application menu
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
