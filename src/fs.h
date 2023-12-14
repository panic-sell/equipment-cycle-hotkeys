// Filesystem wrappers.
#pragma once

namespace ech {
namespace fs {

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

/// Returns false on failure.
inline bool
Remove(const std::filesystem::path& fp) {
    std::error_code ec;
    return std::filesystem::remove(fp, ec);
}

/// Lists filenames inside `dir` and pushes them to `buf`. Returns false on failure.
inline bool
ListDirectoryToBuffer(const std::filesystem::path& dir, std::vector<std::string>& buf) {
    std::error_code e;
    auto it = std::filesystem::directory_iterator(dir, e);
    if (e) {
        return false;
    }
    for (const auto& entry : it) {
        buf.emplace_back(entry.path().filename().string());
    }
    return true;
}

}  // namespace fs
}  // namespace ech
