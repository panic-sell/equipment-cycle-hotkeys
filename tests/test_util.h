#pragma once

namespace ech {

class Tempdir {
  public:
    Tempdir(const Tempdir&) = delete;
    Tempdir& operator=(const Tempdir&) = delete;
    Tempdir(Tempdir&&) = delete;
    Tempdir& operator=(Tempdir&&) = delete;

    Tempdir()
        : path_(fmt::format(
            "{}/" ECH_NAME "_{}", std::filesystem::temp_directory_path().string(), std::rand()
        )) {
        std::filesystem::create_directory(path_);
    }

    ~Tempdir() {
        std::filesystem::remove_all(path_);
    }

    /// Does not contain trailing slash.
    const std::string&
    path() const {
        return path_;
    }

  private:
    /// Seeds RNG.
    static inline const int _ = []() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        return 0;
    }();

    std::string path_;
};

}  // namespace ech
