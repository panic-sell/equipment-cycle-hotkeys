#include "fs.h"
#include "hotkeys.h"
#include "keys.h"
#include "test_util.h"
#include "ui_state.h"

namespace ech {

template <typename Q>
void
CompareHotkeys(const Hotkeys<Q>& a, const Hotkeys<Q>& b) {
    auto sz = std::min(a.vec().size(), b.vec().size());
    for (size_t i = 0; i < sz; i++) {
        CAPTURE(i);
        const Hotkey<Q>& ah = a.vec()[i];
        const Hotkey<Q>& bh = b.vec()[i];
        REQUIRE(ah.name == bh.name);
        REQUIRE(ah.keysets.vec() == bh.keysets.vec());
        REQUIRE(ah.equipsets.vec() == bh.equipsets.vec());
        REQUIRE(ah.equipsets.selected() == bh.equipsets.selected());
    }
    REQUIRE(a.selected() == b.selected());
    REQUIRE(a.vec().size() == b.vec().size());
}

template <typename Q>
void
CompareHotkeysUI(const HotkeysUI<Q>& a, const HotkeysUI<Q>& b) {
    auto sz = std::min(a.size(), b.size());
    for (size_t i = 0; i < sz; i++) {
        CAPTURE(i);
        const HotkeyUI<Q>& ah = a[i];
        const HotkeyUI<Q>& bh = b[i];
        REQUIRE(ah.name == bh.name);
        REQUIRE(ah.keysets == bh.keysets);
        REQUIRE(ah.equipsets == bh.equipsets);
    }
    REQUIRE(a.size() == b.size());
}

TEST_CASE("HotkeysUI from Hotkeys") {
    auto hotkeys = Hotkeys(std::vector<Hotkey<std::string_view>>{
        {
            .name = "",
            .keysets = {},
            .equipsets = Equipsets<std::string_view>({"a"}, 0),
        },
        {
            .name = "hk",
            .keysets = Keysets({{1}, {2, 3}}),
            .equipsets = Equipsets<std::string_view>({}, 0),
        },
        {
            .name = "hotkey",
            .keysets = Keysets({{1, 2}, {11, 12}}),
            .equipsets = Equipsets<std::string_view>({"a", "b", "c", ""}, 1),
        },
    });

    auto got = HotkeysUI(hotkeys);
    auto want = HotkeysUI<std::string_view>{
        {
            .name = "",
            .keysets = {},
            .equipsets = {"a"},
        },
        {
            .name = "hk",
            .keysets = {{1}, {2, 3}},
            .equipsets = {},
        },
        {
            .name = "hotkey",
            .keysets = {{1, 2}, {11, 12}},
            .equipsets = {"a", "b", "c", ""},
        },
    };

    CompareHotkeysUI(got, want);
}

TEST_CASE("HotkeysUI to Hotkeys") {
    auto ir = HotkeysUI<std::string_view>{
        {
            .name = "",
            .keysets = {},
            .equipsets = {"a"},
        },
        {
            .name = "hotkeys whose keysets and equipsets are both empty get pruned",
            .keysets = {},
            .equipsets = {},
        },
        {
            .name = "hk",
            .keysets = {{1}, {2, 3}},
            .equipsets = {},
        },
        {
            .name = "hotkey",
            // Keyset constructor will sort invalid keys to the end.
            .keysets = {{1, 0, 2, 0}, {11, 12}},
            .equipsets = {"a", "b", "c", ""},
        },
    };

    auto got = ir.Into();
    auto want = Hotkeys(std::vector<Hotkey<std::string_view>>{
        {
            .name = "",
            .keysets = {},
            .equipsets = Equipsets<std::string_view>({"a"}, 0),
        },
        {
            .name = "hk",
            .keysets = Keysets({{1}, {2, 3}}),
            .equipsets = Equipsets<std::string_view>({}, 0),
        },
        {
            .name = "hotkey",
            .keysets = Keysets({{1, 2}, {11, 12}}),
            .equipsets = Equipsets<std::string_view>({"a", "b", "c", ""}, 0),
        },
    });
    CompareHotkeys(got, want);
}

TEST_CASE("HotkeysUI equipset conversions") {
    auto ir = HotkeysUI<std::string_view>{
        {
            .keysets = {},
            .equipsets = {"a"},
        },
        {
            .keysets = {{1, 14, 15}},
            .equipsets = {"a", "bb", "ccc"},
        },
    };

    auto got = ir.ConvertEquipset(std::mem_fn(&std::string_view::size));
    auto want = HotkeysUI<size_t>{
        {
            .keysets = {},
            .equipsets = {1},
        },
        {
            .keysets = {{1, 14, 15}},
            .equipsets = {1, 2, 3},
        },
    };
    CompareHotkeysUI(got, want);
}

TEST_CASE("UI tests") {
    auto td = Tempdir();
    auto ui = UI(td.path());
    auto ui_nonexistent_profile_dir = UI("8df229d6-f338-4308-9fc4-e1f3c3e188cf");
    ui.Activate();
    ui_nonexistent_profile_dir.Activate();

    SECTION("GetProfilePath") {
        REQUIRE(ui.GetProfilePath("") == td.path() + "/.json");
        REQUIRE(ui.GetProfilePath("abc") == td.path() + "/abc.json");
    }

    SECTION("GetSavedProfiles") {
        SECTION("nonexistent dir") {
            REQUIRE(ui_nonexistent_profile_dir.GetSavedProfiles().empty());
        }

        SECTION("empty dir") {
            REQUIRE(ui.GetSavedProfiles().empty());
        }

        SECTION("normal") {
            REQUIRE(fs::WriteFile(td.path() + "/.json", "")
            );  // empty profile name is filtered away
            REQUIRE(fs::WriteFile(td.path() + "/abc.json", ""));
            REQUIRE(fs::WriteFile(td.path() + "/def.JSoN", ""));
            REQUIRE(fs::WriteFile(td.path() + "/xyz.txt", ""));
            REQUIRE(fs::EnsureDirExists(td.path() + "/A_folder.json"));
            REQUIRE(fs::EnsureDirExists(td.path() + "/some_folder.json"));

            auto sorted_cache = ui.GetSavedProfiles();
            std::sort(sorted_cache.begin(), sorted_cache.end());
            REQUIRE(
                sorted_cache == std::vector<std::string>{"A_folder", "abc", "def", "some_folder"}
            );
        }
    }

    SECTION("NormalizeExportName") {
        const auto& [orig, want] = GENERATE(
            std::pair("asdf", "asdf"),
            std::pair("AS/\\df_c.json", "ASdf_cjson"),
            std::pair("  a  b  ", "a  b"),
            std::pair("     ", ""),
            std::pair("   ✒  . ", "")
        );
        ui.export_name = orig;
        ui.NormalizeExportName();
        REQUIRE(ui.export_name == want);
    }

    SECTION("GetSavedProfileMatchingExportName") {
        REQUIRE(fs::WriteFile(td.path() + "/aaa.json", ""));
        REQUIRE(fs::WriteFile(td.path() + "/bbb.json", ""));
        REQUIRE(fs::WriteFile(td.path() + "/ccc.json", ""));
        REQUIRE(fs::EnsureDirExists(td.path() + "/DDD.json"));

        const auto& [profile_for_export, want_match] = GENERATE(
            std::pair("aaa", "aaa"),
            std::pair("AaA", "aaa"),
            std::pair("a", nullptr),
            std::pair("ddd", "DDD")
        );

        ui.export_name = profile_for_export;
        auto found = ui.GetSavedProfileMatchingExportName();
        if (want_match) {
            REQUIRE(found);
            REQUIRE(*found == want_match);
        } else {
            REQUIRE(!found);
        }
    }
}

}  // namespace ech
