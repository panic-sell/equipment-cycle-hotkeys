// Filesystem wrappers.
#pragma once

namespace ech {
namespace fs {

#ifndef ECH_UI_DEV
inline constexpr const char* kSettingsPath = "Data/SKSE/Plugins/EquipmentCycleHotkeys.json";
inline constexpr const char* kImGuiIniPath = "Data/SKSE/Plugins/EquipmentCycleHotkeys_imgui.ini";
inline constexpr const char* kProfileDir = "Data/SKSE/Plugins/EquipmentCycleHotkeys";
#else
inline constexpr const char* kSettingsPath = ".ech/EquipmentCycleHotkeys.json";
inline constexpr const char* kImGuiIniPath = ".ech/EquipmentCycleHotkeys_imgui.ini";
inline constexpr const char* kProfileDir = ".ech/EquipmentCycleHotkeys";
#endif

/// Returns nullopt on failure.
inline std::optional<std::string>
Read(const std::filesystem::path& fp) {
    auto f = std::ifstream(fp);
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/// Will create intermediate directories as needed. Returns false on failure.
inline bool
Write(const std::filesystem::path& fp, std::string_view contents) {
    std::error_code ec;
    std::filesystem::create_directories(fp.parent_path(), ec);
    if (ec) {
        return false;
    }

    auto f = std::ofstream(fp);
    if (!f.is_open()) {
        return false;
    }
    f << contents;
    return true;
}

/// Returns false on failure (including file-does-not-exist).
inline bool
Remove(const std::filesystem::path& fp) {
    std::error_code ec;
    return std::filesystem::remove(fp, ec);
}

/// For all items inside `dir`, puts their names into `buf`. A nonexistent `dir` is treated like an
/// empty directory. Returns false on failure.
inline bool
ListDirectoryToBuffer(const std::filesystem::path& dir, std::vector<std::string>& buf) {
    std::error_code ec;
    auto it = std::filesystem::directory_iterator(dir, ec);
    if (ec.value() == ERROR_PATH_NOT_FOUND) {
        return true;
    }
    if (ec) {
        return false;
    }
    for (const auto& entry : it) {
        buf.emplace_back(entry.path().filename().string());
    }
    return true;
}

}  // namespace fs
}  // namespace ech
