#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hotkeys.h"
#include "keys.h"
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
        REQUIRE(ah.equipsets.active() == bh.equipsets.active());
    }
    REQUIRE(a.active() == b.active());
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

template <typename Q>
requires(std::equality_comparable<Q>)
bool
operator==(const HotkeyUI<Q>& a, const HotkeyUI<Q>& b) {
    return a.name == b.name && a.keysets == b.keysets && a.equipsets == b.equipsets
           && a.active_equipset == b.active_equipset;
}

template <typename Q>
requires(std::equality_comparable<Q>)
bool
operator==(const HotkeysUI<Q>& a, const HotkeysUI<Q>& b) {
    return a.hotkeys == b.hotkeys && a.active_hotkey == b.active_hotkey;
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
            // active_equipsets defaults to 0
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

}  // namespace ech
