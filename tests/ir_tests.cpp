#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hotkeys.h"
#include "ir.h"
#include "keys.h"

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

template <typename K, typename Q>
void
CompareHotkeysIR(const HotkeysUI<K, Q>& a, const HotkeysUI<K, Q>& b) {
    auto sz = std::min(a.hotkeys.size(), b.hotkeys.size());
    for (size_t i = 0; i < sz; i++) {
        CAPTURE(i);
        const HotkeyUI<K, Q>& ah = a.hotkeys[i];
        const HotkeyUI<K, Q>& bh = b.hotkeys[i];
        REQUIRE(ah.name == bh.name);
        REQUIRE(ah.keysets == bh.keysets);
        REQUIRE(ah.equipsets == bh.equipsets);
        REQUIRE(ah.active_equipset == bh.active_equipset);
    }
    REQUIRE(a.active_hotkey == b.active_hotkey);
    REQUIRE(a.hotkeys.size() == b.hotkeys.size());
}

template <typename K, typename Q>
requires(std::equality_comparable<K> && std::equality_comparable<Q>)
bool
operator==(const HotkeyUI<K, Q>& a, const HotkeyUI<K, Q>& b) {
    return a.name == b.name && a.keysets == b.keysets && a.equipsets == b.equipsets
           && a.active_equipset == b.active_equipset;
}

template <typename K, typename Q>
requires(std::equality_comparable<K> && std::equality_comparable<Q>)
bool
operator==(const HotkeysUI<K, Q>& a, const HotkeysUI<K, Q>& b) {
    return a.hotkeys == b.hotkeys && a.active_hotkey == b.active_hotkey;
}

TEST_CASE("HotkeysUI serialize") {
    auto hotkeys = Hotkeys(
        std::vector<Hotkey<std::string_view>>{
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
        },
        2
    );

    SECTION("discard active status") {
        auto got = HotkeysUI(hotkeys);
        auto want = HotkeysUI(
            std::vector<HotkeyUI<Keyset, std::string_view>>{
                {
                    .name = "",
                    .keysets = {},
                    .equipsets = {"a"},
                    .active_equipset = 0,
                },
                {
                    .name = "hk",
                    .keysets = {{1}, {2, 3}},
                    .equipsets = {},
                    .active_equipset = 0,
                },
                {
                    .name = "hotkey",
                    .keysets = {{1, 2}, {11, 12}},
                    .equipsets = {"a", "b", "c", ""},
                    .active_equipset = 0,
                },
            },
            size_t(-1)
        );
        CompareHotkeysIR(got, want);
    }

    SECTION("persist active status") {
        auto got = HotkeysUI(hotkeys, false);
        auto want = HotkeysUI(
            std::vector<HotkeyUI<Keyset, std::string_view>>{
                {
                    .name = "",
                    .keysets = {},
                    .equipsets = {"a"},
                    .active_equipset = 0,
                },
                {
                    .name = "hk",
                    .keysets = {{1}, {2, 3}},
                    .equipsets = {},
                    .active_equipset = 0,
                },
                {
                    .name = "hotkey",
                    .keysets = {{1, 2}, {11, 12}},
                    .equipsets = {"a", "b", "c", ""},
                    .active_equipset = 1,
                },
            },
            2
        );
        CompareHotkeysIR(got, want);
    }
}

TEST_CASE("HotkeysUI deserialize") {
    auto ir = HotkeysUI(
        std::vector<HotkeyUI<Keyset, std::string_view>>{
            {
                .name = "",
                .keysets = {},
                .equipsets = {"a"},
                .active_equipset = 0,
            },
            {
                .name = "hotkeys whose keysets and equipsets are both empty get pruned",
                .keysets = {},
                .equipsets = {},
                .active_equipset = 1,
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
                .active_equipset = 1,
            },
        },
        2
    );

    SECTION("discard active status") {
        auto got = ir.Into();
        auto want = Hotkeys(
            std::vector<Hotkey<std::string_view>>{
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
            },
            size_t(-1)  // Hotkeys constructor caps this at # of hotkeys.
        );
        CompareHotkeys(got, want);
    }

    SECTION("persist active status") {
        auto got = ir.Into(false);
        auto want = Hotkeys(
            std::vector<Hotkey<std::string_view>>{
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
            },
            2
        );
        CompareHotkeys(got, want);
    }
}

TEST_CASE("HotkeysUI keyset/equipset conversions") {
    auto ir = HotkeysUI(std::vector<HotkeyUI<uint32_t, std::string_view>>{
        {
            .keysets = {},
            .equipsets = {"a"},
        },
        {
            .keysets = {0, 14, 15},
            .equipsets = {"a", "bb", "ccc"},
        },
    });

    SECTION("convert keyset") {
        auto got = ir.ConvertKeyset(KeycodeName);
        auto want = HotkeysUI(std::vector<HotkeyUI<std::string_view, std::string_view>>{
            {
                .keysets = {},
                .equipsets = {"a"},
            },
            {
                .keysets = {"", "Backspace", "Tab"},
                .equipsets = {"a", "bb", "ccc"},
            },
        });
        CompareHotkeysIR(got, want);
    }

    SECTION("convert equipset") {
        auto got = ir.ConvertEquipset(std::mem_fn(&std::string_view::size));
        auto want = HotkeysUI(std::vector<HotkeyUI<uint32_t, size_t>>{
            {
                .keysets = {},
                .equipsets = {1},
            },
            {
                .keysets = {0, 14, 15},
                .equipsets = {1, 2, 3},
            },
        });
        CompareHotkeysIR(got, want);
    }
}

}  // namespace ech
