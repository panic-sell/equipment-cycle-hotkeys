// SKSE plugin entry point.
#include "event_handler.h"
#include "fs.h"
#include "serde.h"
#include "settings.h"
#include "ui_drawing.h"
#include "ui_plumbing.h"

namespace {

using namespace ech;

ui::Context gUICtx;
Hotkeys<> gHotkeys = Hotkeys({
    {
        .name = "1",
        .keysets = Keysets({{KeycodeFromName("1")}}),
    },
    {
        .name = "2",
        .keysets = Keysets({{KeycodeFromName("2")}}),
    },
    {
        .name = "3",
        .keysets = Keysets({{KeycodeFromName("3")}}),
    },
    {
        .name = "4",
        .keysets = Keysets({{KeycodeFromName("4")}}),
    },
});

void
InitLogging(const SKSE::PluginDeclaration& plugin_decl) {
    auto log_dir = SKSE::log::log_directory();
    if (!log_dir) {
        SKSE::stl::report_and_fail("failed to get SKSE logs directory");
    }
    log_dir->append(plugin_decl.GetName()).replace_extension("log");

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_dir->string(), true);
    auto logger = std::make_shared<spdlog::logger>("logger", std::move(sink));

    logger->flush_on(spdlog::level::level_enum::trace);
    logger->set_level(spdlog::level::level_enum::trace);
    // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");

    spdlog::set_default_logger(std::move(logger));
}

void
InitState() {
    auto settings = fs::Read(fs::kSettingsPath)
                        .and_then([](std::string&& s) { return Deserialize<Settings>(s); })
                        .or_else([]() {
                            SKSE::log::warn(
                                "failed to parse settings file '{}', falling back to defaults",
                                fs::kSettingsPath
                            );
                            return std::optional(Settings());
                        });

    if (auto res = ui::Init(gUICtx, gHotkeys, std::move(*settings)); !res) {
        SKSE::stl::report_and_fail(res.error());
    }

    if (auto res = EventHandler::Register(gHotkeys); !res) {
        SKSE::stl::report_and_fail(res.error());
    }
}

}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    const auto* plugin_decl = SKSE::PluginDeclaration::GetSingleton();
    if (!plugin_decl) {
        SKSE::stl::report_and_fail("failed to get SKSE plugin declaration");
    }

    InitLogging(*plugin_decl);
    SKSE::Init(skse);

    const auto* msg_interface = SKSE::GetMessagingInterface();
    constexpr auto on_msg = [](SKSE::MessagingInterface::Message* msg) {
        if (msg && msg->type == SKSE::MessagingInterface::kInputLoaded) {
            InitState();
        }
    };
    if (!msg_interface || !msg_interface->RegisterListener(on_msg)) {
        SKSE::stl::report_and_fail("failed to register SKSE message listener");
    }

    SKSE::log::info("{} loaded", plugin_decl->GetName());
    return true;
}
