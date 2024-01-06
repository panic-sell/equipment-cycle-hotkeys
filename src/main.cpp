// SKSE plugin entry point.
#include "fs.h"
#include "hotkeys.h"
#include "input_handler.h"
#include "serde.h"
#include "settings.h"
#include "ui_plumbing.h"
#include "ui_state.h"

namespace {

using namespace ech;

auto gSettings = Settings();
auto gHotkeys = Hotkeys<>();
auto gHotkeysMutex = std::mutex();
auto gUI = UI();
auto gUIMutex = std::mutex();

void
InitSettings() {
    auto settings = fs::ReadFile(fs::kSettingsPath).and_then([](std::string&& s) {
        return Deserialize<Settings>(s);
    });
    if (!settings) {
        SKSE::log::warn("'{}' cannot be parsed, using default settings", fs::kSettingsPath);
        return;
    }
    gSettings = std::move(*settings);
}

void
InitLogging(const SKSE::PluginDeclaration& plugin_decl) {
    auto log_dir = SKSE::log::log_directory();
    if (!log_dir) {
        SKSE::stl::report_and_fail("cannot get SKSE logs directory");
    }
    log_dir->append(plugin_decl.GetName()).replace_extension("log");

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_dir->string(), true);
    auto logger = std::make_shared<spdlog::logger>("logger", std::move(sink));

    auto level = spdlog::level::from_str(gSettings.log_level);
    if (level == spdlog::level::off && gSettings.log_level != "off") {
        level = spdlog::level::info;
    }
    logger->flush_on(level);
    logger->set_level(level);
    // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");

    spdlog::set_default_logger(std::move(logger));
}

void
InitSKSEMessaging(const SKSE::MessagingInterface& mi) {
    constexpr auto listener = [](SKSE::MessagingInterface::Message* msg) -> void {
        if (!msg || msg->type != SKSE::MessagingInterface::kInputLoaded) {
            return;
        }
        if (auto res = ui::Init(gHotkeys, gHotkeysMutex, gUI, gUIMutex, gSettings); !res) {
            SKSE::stl::report_and_fail(res.error());
        }
        if (auto res = InputHandler::Init(gHotkeys, gHotkeysMutex); !res) {
            SKSE::stl::report_and_fail(res.error());
        }
    };

    if (!mi.RegisterListener(listener)) {
        SKSE::stl::report_and_fail("cannot register SKSE message listener");
    }
}

void
InitSKSESerialization(const SKSE::SerializationInterface& si) {
    static constexpr auto on_save = [](SKSE::SerializationInterface* si) -> void {
        if (!si) {
            SKSE::log::error("SerializationInterface save callback called with null pointer");
            return;
        }

        auto lock = std::lock_guard(gHotkeysMutex);
        if (gHotkeys.vec().empty()) {
            return;
        }
        auto s = Serialize(gHotkeys);
        if (!si->WriteRecord('DATA', 1, s.c_str(), static_cast<uint32_t>(s.size()))) {
            SKSE::log::error("cannot serialize hotkeys data to SKSE cosave");
            return;
        }

        SKSE::log::debug("active hotkeys saved to SKSE cosave");
    };

    static constexpr auto on_load = [](SKSE::SerializationInterface* si) -> void {
        if (!si) {
            SKSE::log::error("SerializationInterface load callback called with null pointer");
            return;
        }

        auto lock = std::scoped_lock(gHotkeysMutex, gUIMutex);
        gHotkeys = {};
        uint32_t type;
        uint32_t version;  // unused
        uint32_t length;
        while (si->GetNextRecordInfo(type, version, length)) {
            if (type != 'DATA') {
                SKSE::log::warn("unknown record type '{}' in SKSE cosave", type);
                continue;
            }

            std::string s;
            s.reserve(length);
            for (uint32_t i = 0; i < length; i++) {
                char c;
                si->ReadRecordData(&c, 1);
                s.push_back(c);
            }
            auto hotkeys = Deserialize<Hotkeys<>>(s);
            if (!hotkeys) {
                SKSE::log::error("cannot deserialize hotkeys data from SKSE cosave");
                continue;
            }
            gHotkeys = std::move(*hotkeys);
            SKSE::log::debug("active hotkeys loaded from SKSE cosave");
        }

        gUI.Deactivate();
        gUI.hotkey_in_focus = 0;
    };

    static constexpr auto on_revert = [](SKSE::SerializationInterface* si) -> void {
        if (!si) {
            SKSE::log::error("SerializationInterface revert callback called with null pointer");
            return;
        }

        auto lock = std::scoped_lock(gHotkeysMutex, gUIMutex);
        gHotkeys = {};
        gUI.Deactivate();
        gUI.hotkey_in_focus = 0;
        SKSE::log::debug("active hotkeys discarded");
    };

    si.SetUniqueID('ECH?');
    si.SetSaveCallback(on_save);
    si.SetLoadCallback(on_load);
    si.SetRevertCallback(on_revert);
}

}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    const auto* plugin_decl = SKSE::PluginDeclaration::GetSingleton();
    if (!plugin_decl) {
        SKSE::stl::report_and_fail("cannot get SKSE plugin declaration");
    }

    InitSettings();
    InitLogging(*plugin_decl);
    SKSE::Init(skse);

    const auto* mi = SKSE::GetMessagingInterface();
    const auto* si = SKSE::GetSerializationInterface();
    if (!mi) {
        SKSE::stl::report_and_fail("cannot get SKSE messaging interface");
    }
    if (!si) {
        SKSE::stl::report_and_fail("cannot get SKSE serialization interface");
    }

    InitSKSEMessaging(*mi);
    InitSKSESerialization(*si);

    SKSE::log::info(
        "{} {}.{}.{} loaded",
        plugin_decl->GetName(),
        plugin_decl->GetVersion().major(),
        plugin_decl->GetVersion().minor(),
        plugin_decl->GetVersion().patch()
    );
    return true;
}
