#include "equipsets.h"
#include "hotkeys.h"

namespace ech {

using TestEquipsets = Equipsets<std::string_view>;
using TestHotkey = Hotkey<std::string_view>;
using TestHotkeys = Hotkeys<std::string_view>;

void
AssertSelectNext(
    TestHotkeys& hotkeys,
    std::span<const Keystroke> keystrokes,
    Keypress select_next_result,
    std::string_view final_selected_equipset
) {
    REQUIRE(hotkeys.SelectNextEquipset(keystrokes) == select_next_result);
    const auto* es = hotkeys.GetSelectedEquipset();
    REQUIRE(es);
    REQUIRE(*es == final_selected_equipset);
}

TEST_CASE("Hotkeys ctor") {
    auto hotkeys_v = std::vector<TestHotkey>{
        {.keysets = {}, .equipsets = {}},  // empty, gets pruned
        {.keysets = {}, .equipsets = TestEquipsets({"a1"})},
        {.keysets = Keysets({{1}}), .equipsets = {}},
        {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1"})},
    };

    SECTION("normal") {
        auto hotkeys = TestHotkeys(hotkeys_v);
        REQUIRE(hotkeys.vec().size() == hotkeys_v.size() - 1);
        REQUIRE(!hotkeys.GetSelectedEquipset());
    }

    SECTION("specify initial selection") {
        // Pruned hotkey causes other hotkey indices to shift down by 1.
        auto hotkeys = TestHotkeys(hotkeys_v, 2);
        const auto* es = hotkeys.GetSelectedEquipset();
        REQUIRE(es);
        REQUIRE(*es == "b1");
    }

    SECTION("initial selection becomes oob") {
        auto hotkeys = TestHotkeys(hotkeys_v, 3);
        REQUIRE(!hotkeys.GetSelectedEquipset());
    }
}

TEST_CASE("Hotkeys press") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2"})},
            {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1", "b2"})},
            {.keysets = Keysets({{3}}), .equipsets = TestEquipsets({"c1"})},
        },
        0
    );

    SECTION("press currently selected hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "a2");
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "a1");
    }

    SECTION("press non-selected hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "b1");
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "b2");
    }
}

TEST_CASE("Hotkeys hold") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2", "a3"}, 1)},
            {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1", "b2", "a3"}, 1)},
        },
        0
    );

    SECTION("hold currently selected hotkey") {
        auto ks_press = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        auto ks_hold = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold),
        };

        AssertSelectNext(hotkeys, ks_hold, Keypress::kHold, "a1");
        AssertSelectNext(hotkeys, ks_press, Keypress::kPress, "a2");
        AssertSelectNext(hotkeys, ks_hold, Keypress::kHold, "a1");  // press would have yielded a3
    }

    SECTION("hold non-selected hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "b1");
    }
}

// Semi-holds do not change selected hotkeys/equipsets.
TEST_CASE("Hotkeys almost hold") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2", "a3"}, 2)},
            {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1", "b2", "a3"})},
        },
        0
    );

    SECTION("almost hold selected hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold - 0.01f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kSemihold, "a3");
    }

    SECTION("almost hold non-selected hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, kKeypressHoldThreshold - 0.01f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kSemihold, "a3");
    }
}

TEST_CASE("Hotkeys no match") {
    auto hotkeys = TestHotkeys(
        {
            // Empty, gets pruned.
            {.keysets = {}, .equipsets = {}},

            // Empty keysets, won't match any keystroke input.
            {.keysets = {}, .equipsets = TestEquipsets({"a1"})},

            // Empty equipsets, ignored for matching.
            {.keysets = Keysets({{1}}), .equipsets = {}},
            {.keysets = Keysets({{1}, {2}}), .equipsets = TestEquipsets({"b1", "b2"})},
            {.keysets = Keysets({{2}}), .equipsets = {}},
        },
        2
    );

    const auto* es_orig = hotkeys.GetSelectedEquipset();
    REQUIRE(es_orig);
    REQUIRE(*es_orig == "b1");

    auto ks = std::vector<Keystroke>();
    AssertSelectNext(hotkeys, ks, Keypress::kNone, *es_orig);

    ks = std::vector<Keystroke>{*Keystroke::New(9, 0.f)};
    AssertSelectNext(hotkeys, ks, Keypress::kNone, *es_orig);
}

TEST_CASE("Hotkeys deselect") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2"}, 1)},
        },
        0
    );

    const auto* es = hotkeys.GetSelectedEquipset();
    REQUIRE(es);
    REQUIRE(*es == "a2");

    hotkeys.Deselect();
    REQUIRE(!hotkeys.GetSelectedEquipset());

    auto ks = std::vector<Keystroke>{*Keystroke::New(1, 0.f)};
    // would be a1 if hotkeys wasn't deselected beforehand
    AssertSelectNext(hotkeys, ks, Keypress::kPress, "a2");
}

TEST_CASE("Hotkeys hotkey precedence") {
    auto hotkeys = TestHotkeys({
        {
            .keysets = Keysets({
                {1},
                {1, 2},
            }),
            .equipsets = TestEquipsets({"a1", "a2"}),
        },
        {
            .keysets = Keysets({
                {1, 2},
                {1, 3},
                {2, 3},
            }),
            .equipsets = TestEquipsets({"b1", "b2"}),
        },
    });

    auto ks = std::vector<Keystroke>{
        *Keystroke::New(1, 0.f),
        *Keystroke::New(2, 0.f),
    };
    AssertSelectNext(hotkeys, ks, Keypress::kPress, "a1");

    // {1} is a subset of {1, 3}, so the first hotkey gets picked up even though {1, 3}
    // also matches the second hotkey.
    ks = std::vector<Keystroke>{
        *Keystroke::New(1, 0.f),
        *Keystroke::New(3, 0.f),
    };
    AssertSelectNext(hotkeys, ks, Keypress::kPress, "a2");

    ks = std::vector<Keystroke>{
        *Keystroke::New(1, 0.f),
        *Keystroke::New(2, 0.f),
        *Keystroke::New(3, 0.f),
    };
    AssertSelectNext(hotkeys, ks, Keypress::kPress, "a1");
}

TEST_CASE("Hotkeys keystroke concurrency") {
    auto hotkeys = TestHotkeys(
        {
            {
                .keysets = Keysets({
                    {1},
                    {1, 2},
                }),
                .equipsets = TestEquipsets({"a1", "a2", "3"}),
            },
            {
                .keysets = Keysets({
                    {3},
                    {1, 2},
                    {2, 3},
                }),
                .equipsets = TestEquipsets({"b1", "b2"}, 1),
            },
        },
        0
    );

    SECTION("infinite hold of earlier hotkey blocks selection of later ones") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "a2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold - .01f),
            *Keystroke::New(3, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kSemihold, "a2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold),
            *Keystroke::New(3, kKeypressHoldThreshold - .01f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold),
            *Keystroke::New(3, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, kKeypressHoldThreshold),
            *Keystroke::New(3, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "a1");
    }

    SECTION("selection of earlier hotkey takes precedence over infinite hold of later ones") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(3, 0.f),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "b2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(3, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "b1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
            *Keystroke::New(3, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kPress, "a1");

        // Letting go of {1} causes {3} to take over again.
        ks = std::vector<Keystroke>{
            *Keystroke::New(3, kKeypressHoldThreshold),
        };
        AssertSelectNext(hotkeys, ks, Keypress::kHold, "b1");
    }
}

TEST_CASE("Hotkeys structural equality") {
    struct Testcase {
        std::string_view name;
        TestHotkeys a;
        TestHotkeys b;
        bool want;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "empty",
            .a = {},
            .b = {},
            .want = true,
        },
        Testcase{
            .name = "same_structure",
            .a = TestHotkeys(
                {
                    {
                        .name = "hk1",
                        .keysets = Keysets({{1}}),
                        .equipsets = TestEquipsets({"a", "b", "c"}, 0),
                    },
                },
                0
            ),
            .b = TestHotkeys({
                {
                    .name = "hk1",
                    .keysets = Keysets({{1}}),
                    .equipsets = TestEquipsets({"a", "b", "c"}, 1),
                },
            }),
            .want = true,
        },
        Testcase{
            .name = "different_names",
            .a = TestHotkeys(
                {
                    {
                        .name = "hk1",
                        .keysets = Keysets({{1}}),
                        .equipsets = TestEquipsets({"a", "b", "c"}, 0),
                    },
                },
                0
            ),
            .b = TestHotkeys({
                {
                    .name = "hk2",
                    .keysets = Keysets({{1}}),
                    .equipsets = TestEquipsets({"a", "b", "c"}, 1),
                },
            }),
            .want = false,
        },
        Testcase{
            .name = "different_keysets",
            .a = TestHotkeys(
                {
                    {
                        .name = "hk1",
                        .keysets = {},
                        .equipsets = TestEquipsets({"a", "b", "c"}, 0),
                    },
                },
                0
            ),
            .b = TestHotkeys({
                {
                    .name = "hk1",
                    .keysets = Keysets({{1}}),
                    .equipsets = TestEquipsets({"a", "b", "c"}, 1),
                },
            }),
            .want = false,
        },
        Testcase{
            .name = "different_equipsets",
            .a = TestHotkeys(
                {
                    {
                        .name = "hk1",
                        .keysets = Keysets({{1}}),
                        .equipsets = TestEquipsets({"a", "b"}, 0),
                    },
                },
                0
            ),
            .b = TestHotkeys({
                {
                    .name = "hk1",
                    .keysets = Keysets({{1}}),
                    .equipsets = TestEquipsets({"a", "b", "c"}, 1),
                },
            }),
            .want = false,
        },
        Testcase{
            .name = "different_hotkeys",
            .a = {},
            .b = TestHotkeys({
                {
                    .name = "hk2",
                    .keysets = Keysets({{1}}),
                    .equipsets = TestEquipsets({"a", "b", "c"}, 1),
                },
            }),
            .want = false,
        }
    );

    CAPTURE(testcase.name);
    auto got = testcase.a.StructurallyEquals(testcase.b);
    REQUIRE(got == testcase.want);
}

}  // namespace ech
