#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hotkeys.h"
#include "ir.h"
#include "keys.h"

namespace ech {

template <typename T>
void
CompareHotkeys(const Hotkeys<T>& a, const Hotkeys<T>& b) {
    auto sz = std::min(a.vec().size(), b.vec().size());
    for (size_t i = 0; i < sz; i++) {
        CAPTURE(i);
        const Hotkey<T>& ah = a.vec()[i];
        const Hotkey<T>& bh = b.vec()[i];
        REQUIRE(ah.name == bh.name);
        REQUIRE(ah.keysets.vec() == bh.keysets.vec());
        REQUIRE(ah.equipsets.vec() == bh.equipsets.vec());
        REQUIRE(ah.equipsets.active() == bh.equipsets.active());
    }
    REQUIRE(a.active() == b.active());
    REQUIRE(a.vec().size() == b.vec().size());
}

template <typename K, typename E>
void
CompareHotkeysIR(const HotkeysIR<K, E>& a, const HotkeysIR<K, E>& b) {
    auto sz = std::min(a.hotkeys.size(), b.hotkeys.size());
    for (size_t i = 0; i < sz; i++) {
        CAPTURE(i);
        const HotkeyIR<K, E>& ah = a.hotkeys[i];
        const HotkeyIR<K, E>& bh = b.hotkeys[i];
        REQUIRE(ah.name == bh.name);
        REQUIRE(ah.keysets == bh.keysets);
        REQUIRE(ah.equipsets == bh.equipsets);
        REQUIRE(ah.active_equipset == bh.active_equipset);
    }
    REQUIRE(a.active_hotkey == b.active_hotkey);
    REQUIRE(a.hotkeys.size() == b.hotkeys.size());
}

template <typename K, typename E>
requires(std::equality_comparable<K> && std::equality_comparable<E>)
bool
operator==(const HotkeyIR<K, E>& a, const HotkeyIR<K, E>& b) {
    return a.name == b.name && a.keysets == b.keysets && a.equipsets == b.equipsets
           && a.active_equipset == b.active_equipset;
}

template <typename K, typename E>
requires(std::equality_comparable<K> && std::equality_comparable<E>)
bool
operator==(const HotkeysIR<K, E>& a, const HotkeysIR<K, E>& b) {
    return a.hotkeys == b.hotkeys && a.active_hotkey == b.active_hotkey;
}

TEST_CASE("KeysetSer serialize") {
    struct Testcase {
        Keyset input;
        KeysetSer want;
    };

    auto testcase = GENERATE(
        Testcase{.input = {}, .want = {}},
        Testcase{.input = {{1, 2, 3, 4}}, .want = {{"Esc", "1", "2", "3"}}},
        Testcase{.input = {{1, 0, 1, 4}}, .want = {{"Esc", "Esc", "3"}}}
    );
    CAPTURE(testcase.input);
    auto got = KeysetSer::From(testcase.input);
    REQUIRE(got == testcase.want);
}

TEST_CASE("KeysetSer deserialize") {
    struct Testcase {
        KeysetSer input;
        Keyset want;
    };

    auto testcase = GENERATE(
        Testcase{.input = {}, .want = {}},
        Testcase{.input = {{"3", "2", "1", "Esc"}}, .want = {{1, 2, 3, 4}}},
        Testcase{.input = {{"-", "Esc", "-"}}, .want = {{1, 12}}},
        // Only the first 4 items are considered, even if some of them are invalid.
        Testcase{.input = {{"Esc", "asdfasdf", "", "1", "2", "3"}}, .want = {{1, 2}}}
    );
    CAPTURE(testcase.input);
    auto got = testcase.input.To();
    REQUIRE(got == testcase.want);
}

TEST_CASE("HotkeysIR serialize") {
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
        auto got = HotkeysIR(hotkeys);
        auto want = HotkeysIR(
            std::vector<HotkeyIR<Keyset, std::string_view>>{
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
            static_cast<size_t>(-1)
        );
        CompareHotkeysIR(got, want);
    }

    SECTION("persist active status") {
        auto got = HotkeysIR(hotkeys, false);
        auto want = HotkeysIR(
            std::vector<HotkeyIR<Keyset, std::string_view>>{
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

TEST_CASE("HotkeysIR deserialize") {
    auto ir = HotkeysIR(
        std::vector<HotkeyIR<Keyset, std::string_view>>{
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
            3  // Hotkeys constructor caps this at # of hotkeys.
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

TEST_CASE("HotkeysIR keyset/equipset conversions") {
    auto ir = HotkeysIR(std::vector<HotkeyIR<uint32_t, std::string_view>>{
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
        auto want = HotkeysIR(std::vector<HotkeyIR<std::string_view, std::string_view>>{
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
        auto want = HotkeysIR(std::vector<HotkeyIR<uint32_t, size_t>>{
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

TEST_CASE("HotkeysIR json") {
    using K = std::vector<int>;
    using E = std::vector<std::string_view>;
    auto initial_ir = HotkeysIR(
        std::vector<HotkeyIR<K, E>>{
            {
                .name = "a hotkey",
                .keysets = {{}, {1}, {2, 3, 4}},
                .equipsets = {{"a"}, {"bc", "def"}, {}},
                .active_equipset = 1,
            },
            {},
            {
                .name = "empty hotkey",
                .keysets = {},
                .equipsets = {},
                .active_equipset = 2,
            },
        },
        123  // will be clamped to vec.size()
    );

    auto s = nlohmann::json(initial_ir).dump(/*indent=*/1, /*indent_char=*/'\t');
    CAPTURE(s);
    auto j = nlohmann::json::parse(s, nullptr, false);
    REQUIRE(!j.is_discarded());

    auto final_ir = j.template get<HotkeysIR<K, E>>();
    CompareHotkeysIR(final_ir, initial_ir);
    // REQUIRE(final_ir == initial_ir);
}

}  // namespace ech
