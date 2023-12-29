#include "fs.h"

namespace ech {
namespace fs {

class Tempdir {
  public:
    Tempdir(const Tempdir&) = delete;
    Tempdir& operator=(const Tempdir&) = delete;
    Tempdir(Tempdir&&) = delete;
    Tempdir& operator=(Tempdir&&) = delete;

    Tempdir()
        : path_(std::format(
            "{}/EquipmentCycleHotkeys_{}",
            std::filesystem::temp_directory_path().string(),
            std::rand()
        )) {
        std::filesystem::create_directory(path_);
    }

    ~Tempdir() {
        std::filesystem::remove_all(path_);
    }

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

TEST_CASE("fs read/write") {
    Tempdir td;

    // Auto creates the intermediate folder `dir`.
    auto fp = td.path() + "/dir/some_file.txt";
    auto contents = "hi how are you";
    REQUIRE(Write(fp, contents));

    auto read_contents = Read(fp);
    REQUIRE(read_contents);
    REQUIRE(*read_contents == contents);
}

TEST_CASE("fs remove") {
    Tempdir td;

    REQUIRE(Write(td.path() + "/file.txt", ""));
    REQUIRE(Remove(td.path() + "/file.txt"));

    std::filesystem::create_directories(td.path() + "/unnested_dir");
    REQUIRE(Remove(td.path() + "/unnested_dir"));

    std::filesystem::create_directories(td.path() + "/nested_dir/subdir");
    REQUIRE(!Remove(td.path() + "/nested_dir"));
}

TEST_CASE("fs ListDirectory") {
    Tempdir td;

    REQUIRE(Write(td.path() + "/.a_file", ""));
    REQUIRE(Write(td.path() + "/ano.ther..file", ""));
    std::filesystem::create_directories(td.path() + "/a_dir");
    std::filesystem::create_directories(td.path() + "/a.nested.dir");

    std::vector<std::string> got;
    REQUIRE(ListDirectoryToBuffer(td.path(), got));
    std::sort(got.begin(), got.end());

    auto want = std::vector<std::string>{".a_file", "a.nested.dir", "a_dir", "ano.ther..file"};
    REQUIRE(got == want);
}

TEST_CASE("fs ListDirectory file") {
    Tempdir td;

    REQUIRE(Write(td.path() + "/.a_file", ""));

    std::vector<std::string> v;
    REQUIRE(!ListDirectoryToBuffer(td.path() + "/.a_file", v));
}

TEST_CASE("fs ListDirectory nonexistent") {
    std::vector<std::string> v;
    REQUIRE(ListDirectoryToBuffer("lkjahghalu1g193ubfouhojdsbg31801g", v));
    REQUIRE(v.empty());
}

}  // namespace fs
}  // namespace ech
