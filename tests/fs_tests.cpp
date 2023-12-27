#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "fs.h"

namespace ech {
namespace fs {

class Tempdir {
  public:
    Tempdir(const Tempdir&) = delete;
    Tempdir& operator=(const Tempdir&) = delete;
    Tempdir(Tempdir&&) = delete;
    Tempdir& operator=(Tempdir&&) = delete;

    Tempdir() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        path_ = std::filesystem::temp_directory_path();
        path_ /= std::format("EquipmentCycleHotkeys_{}", std::rand());
        std::filesystem::create_directory(path_);
    }

    ~Tempdir() {
        std::filesystem::remove_all(path_);
    }

    const std::filesystem::path&
    path() const {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

TEST_CASE("Read/Write") {
    Tempdir td;

    // Auto creates the intermediate folder `dir`.
    auto fp = td.path() / "dir" / "some_file.txt";
    auto contents = "hi how are you";
    REQUIRE(Write(fp, contents));

    auto read_contents = Read(fp);
    REQUIRE(read_contents);
    REQUIRE(*read_contents == contents);
}

TEST_CASE("Remove") {
    Tempdir td;

    REQUIRE(Write(td.path() / "file.txt", ""));
    REQUIRE(Remove(td.path() / "file.txt"));

    std::filesystem::create_directories(td.path() / "unnested_dir");
    REQUIRE(Remove(td.path() / "unnested_dir"));

    std::filesystem::create_directories(td.path() / "nested_dir" / "subdir");
    REQUIRE(!Remove(td.path() / "nested_dir"));
}

TEST_CASE("ListDirectory") {
    Tempdir td;

    REQUIRE(Write(td.path() / ".a_file", ""));
    REQUIRE(Write(td.path() / "ano.ther..file", ""));
    std::filesystem::create_directories(td.path() / "a_dir");
    std::filesystem::create_directories(td.path() / "a.nested.dir");

    std::vector<std::string> got;
    REQUIRE(ListDirectoryToBuffer(td.path(), got));
    std::sort(got.begin(), got.end());

    auto want = std::vector<std::string>{".a_file", "a.nested.dir", "a_dir", "ano.ther..file"};
    REQUIRE(got == want);
}

TEST_CASE("ListDirectory file") {
    Tempdir td;

    REQUIRE(Write(td.path() / ".a_file", ""));

    std::vector<std::string> v;
    REQUIRE(!ListDirectoryToBuffer(td.path() / ".a_file", v));
}

TEST_CASE("ListDirectory nonexistent") {
    std::vector<std::string> v;
    REQUIRE(ListDirectoryToBuffer("lkjahghalu1g193ubfouhojdsbg31801g", v));
    REQUIRE(v.empty());
}

}  // namespace fs
}  // namespace ech
