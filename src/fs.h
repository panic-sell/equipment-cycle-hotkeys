// Filesystem wrappers.
#pragma once

namespace ech {
namespace fs {
namespace internal {

[[nodiscard]] inline bool
EnsureDirExists(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return !ec;
}

}  // namespace internal

#ifdef ECH_UI_DEV
inline constexpr const char* kProfileDir = ".ech/" ECH_NAME;
inline constexpr const char* kSettingsPath = ".ech/" ECH_NAME ".json";
inline constexpr const char* kImGuiIniPath = ".ech/" ECH_NAME "_imgui.ini";
#else
inline constexpr const char* kProfileDir = "Data/SKSE/Plugins/" ECH_NAME;
inline constexpr const char* kSettingsPath = "Data/SKSE/Plugins/" ECH_NAME ".json";
inline constexpr const char* kImGuiIniPath = "Data/SKSE/Plugins/" ECH_NAME "_imgui.ini";
#endif

inline std::optional<std::filesystem::path>
PathFromStr(std::string_view s) {
    return SKSE::stl::utf8_to_utf16(s).transform([](std::wstring&& ws) {
        return std::filesystem::path(std::move(ws));
    });
}

inline std::optional<std::string>
StrFromPath(const std::filesystem::path& p) {
    return SKSE::stl::utf16_to_utf8(p.c_str());
}

/// Returns nullopt on failure.
inline std::optional<std::string>
ReadFile(std::string_view path) {
    auto fp = PathFromStr(path);
    if (!fp) {
        return std::nullopt;
    }
    auto f = std::ifstream(*fp);
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/// Will create intermediate directories as needed. Returns false on failure.
[[nodiscard]] inline bool
WriteFile(std::string_view path, std::string_view contents) {
    auto fp = PathFromStr(path);
    if (!fp) {
        return false;
    }
    if (!internal::EnsureDirExists(fp->parent_path())) {
        return false;
    }
    auto f = std::ofstream(*fp);
    if (!f.is_open()) {
        return false;
    }
    f << contents;
    return true;
}

/// Returns false on failure. Attempting to remove a nonexistent file is considered a failure.
[[nodiscard]] inline bool
RemoveFile(std::string_view path) {
    auto fp = PathFromStr(path);
    if (!fp) {
        return false;
    }
    auto ec = std::error_code();
    return std::filesystem::remove(*fp, ec);
}

/// Will create intermediate directories as needed. Returns false on failure.
[[nodiscard]] inline bool
EnsureDirExists(std::string_view path) {
    auto p = PathFromStr(path);
    if (!p) {
        return false;
    }
    return internal::EnsureDirExists(*p);
}

/// For all items inside `dir`, puts their names into `buf`. A nonexistent `dir` is treated like an
/// empty directory. Returns false on failure.
[[nodiscard]] inline bool
ListDirToBuf(std::string_view dir_path, std::vector<std::string>& buf) {
    auto dp = PathFromStr(dir_path);
    if (!dp) {
        return false;
    }
    auto ec = std::error_code();
    auto it = std::filesystem::directory_iterator(*dp, ec);
    if (ec.value() == ERROR_PATH_NOT_FOUND) {
        return true;
    }
    if (ec) {
        return false;
    }
    for (const auto& entry : it) {
        auto s = StrFromPath(entry.path().filename());
        if (s) {
            buf.push_back(std::move(*s));
        }
    }
    return true;
}

}  // namespace fs
}  // namespace ech
