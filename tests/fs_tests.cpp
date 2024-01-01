#include "fs.h"
#include "test_util.h"

namespace ech {
namespace fs {

TEST_CASE("fs read/write file") {
    auto td = Tempdir();

    // Auto creates the intermediate folder `dir`.
    auto fp = td.path() + "/dir/some_file.txt";
    auto contents = "hi how are you";
    REQUIRE(WriteFile(fp, contents));

    auto read_contents = ReadFile(fp);
    REQUIRE(read_contents);
    REQUIRE(*read_contents == contents);
}

TEST_CASE("fs ListDirectory") {
    auto td = Tempdir();

    REQUIRE(WriteFile(td.path() + "/.a_file", ""));
    REQUIRE(WriteFile(td.path() + "/ano.ther..file", ""));
    REQUIRE(EnsureDirExists(td.path() + "/a_dir"));
    REQUIRE(EnsureDirExists(td.path() + "/a.nested.dir"));

    std::vector<std::string> got;
    REQUIRE(ListDirToBuf(td.path(), got));
    std::sort(got.begin(), got.end());

    auto want = std::vector<std::string>{".a_file", "a.nested.dir", "a_dir", "ano.ther..file"};
    REQUIRE(got == want);
}

TEST_CASE("fs ListDirectory file") {
    auto td = Tempdir();

    REQUIRE(WriteFile(td.path() + "/.a_file", ""));

    std::vector<std::string> v;
    REQUIRE(!ListDirToBuf(td.path() + "/.a_file", v));
}

TEST_CASE("fs ListDirectory nonexistent") {
    std::vector<std::string> v;
    REQUIRE(ListDirToBuf("lkjahghalu1g193ubfouhojdsbg31801g", v));
    REQUIRE(v.empty());
}

}  // namespace fs
}  // namespace ech
