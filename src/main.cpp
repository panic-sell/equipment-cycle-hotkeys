// SKSE plugin entry point.
#include "event_handler.h"
#include "fs.h"
#include "settings.h"
#include "ui_plumbing.h"

namespace {

using namespace ech;

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
InitSettings() {
    auto settings = fs::Read(fs::kSettingsPath).and_then([](std::string&& s) {
        return Deserialize<Settings>(s);
    });
    if (!settings) {
        SKSE::log::warn(
            "failed to parse settings file '{}', falling back to defaults", fs::kSettingsPath
        );
        return;
    }
    Settings::GetSingleton(&*settings);
}

void
OnMessage(SKSE::MessagingInterface::Message* msg) {
    if (!msg || msg->type != SKSE::MessagingInterface::kInputLoaded) {
        return;
    }

    // UI context and input handler.
    if (auto res = ui::Init(); !res.has_value()) {
        SKSE::stl::report_and_fail(res.error());
    }

    // General event handler.
    if (auto res = EventHandler::GetSingleton().Register(); !res.has_value()) {
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
    InitSettings();
    SKSE::Init(skse);

    const auto* msg_interface = SKSE::GetMessagingInterface();
    if (!msg_interface || !msg_interface->RegisterListener(OnMessage)) {
        SKSE::stl::report_and_fail("failed to register SKSE message listener");
    }

    SKSE::log::info("{} loaded", plugin_decl->GetName());
    return true;
}
