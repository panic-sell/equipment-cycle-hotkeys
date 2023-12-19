#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "equipsets.h"
#include "hotkeys.h"

namespace ech {

using TestEquipsets = Equipsets<std::string_view>;
using TestHotkey = Hotkey<std::string_view>;
using TestHotkeys = Hotkeys<std::string_view>;

TEST_CASE("Hotkeys ctor") {
    auto hotkeys = std::vector<TestHotkey>{
        {.keysets = {}, .equipsets = {}},  // empty, gets pruned
        {.keysets = {}, .equipsets = TestEquipsets({"a1"})},
        {.keysets = Keysets({{1}}), .equipsets = {}},
        {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1"})},
    };

    SECTION("normal") {
        auto hm = TestHotkeys(hotkeys);
        REQUIRE(hm.vec().size() == hotkeys.size() - 1);
        REQUIRE(!hm.GetActiveEquipset());
    }

    SECTION("specify active hotkey index") {
        // Pruned hotkey causes other hotkey indices to shift down by 1.
        auto hm = TestHotkeys(hotkeys, 2);
        REQUIRE(*hm.GetActiveEquipset() == "b1");
    }

    SECTION("active hotkey index becomes oob") {
        auto hm = TestHotkeys(hotkeys, 3);
        REQUIRE(!hm.GetActiveEquipset());
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

    SECTION("press currently active hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "a2");
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "a1");
    }

    SECTION("press inactive hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, 0.f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b1");
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b2");
    }

    SECTION("repeated press of hotkey with a single equipset yields null") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(3, 0.f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "c1");
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(!hotkeys.GetNextEquipset(ks));
    }
}

TEST_CASE("Hotkeys hold") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2", "a3"})},
            {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1", "b2", "a3"}, 1)},
        },
        0
    );

    SECTION("hold currently active hotkey") {
        auto ks_press = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        auto ks_hold = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks_hold) == "a1");
        REQUIRE(*hotkeys.GetNextEquipset(ks_press) == "a2");
        REQUIRE(*hotkeys.GetNextEquipset(ks_hold) == "a1");  // press would have yielded a3
    }

    SECTION("hold inactive hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b1");
    }

    SECTION("repeated hold yields null") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b1");
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(!hotkeys.GetNextEquipset(ks));
    }
}

TEST_CASE("Hotkeys almost hold") {
    auto hotkeys = TestHotkeys(
        {
            {.keysets = Keysets({{1}}), .equipsets = TestEquipsets({"a1", "a2", "a3"}, 2)},
            {.keysets = Keysets({{2}}), .equipsets = TestEquipsets({"b1", "b2", "a3"})},
        },
        0
    );

    SECTION("almost hold active hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold - 0.01f),
        };
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(*hotkeys.GetActiveEquipset() == "a3");
    }

    SECTION("almost hold inactive hotkey") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(2, Keysets::kHoldThreshold - 0.01f),
        };
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        // An "almost hold" does not change the current active hotkey.
        REQUIRE(*hotkeys.GetActiveEquipset() == "a3");
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
            {.keysets = Keysets({{1}, {2}}), .equipsets = TestEquipsets({"b1"})},
            {.keysets = Keysets({{2}}), .equipsets = {}},
        },
        2  // points to b1 after pruning
    );

    auto ks = std::vector<Keystroke>();
    REQUIRE(!hotkeys.GetNextEquipset(ks));

    ks = std::vector<Keystroke>{*Keystroke::New(9, 0.f)};
    REQUIRE(!hotkeys.GetNextEquipset(ks));

    ks = std::vector<Keystroke>{*Keystroke::New(1, 0.f)};
    const auto* es = hotkeys.GetNextEquipset(ks);
    REQUIRE(es);
    REQUIRE(*es == "b1");

    hotkeys.Deactivate();
    ks = std::vector<Keystroke>{*Keystroke::New(2, 0.f)};
    es = hotkeys.GetNextEquipset(ks);
    REQUIRE(es);
    REQUIRE(*es == "b1");
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
    REQUIRE(*hotkeys.GetNextEquipset(ks) == "a1");

    // {1} is a subset of {1, 3}, so the first hotkey gets picked up even though {1, 3}
    // also matches the second hotkey.
    ks = std::vector<Keystroke>{
        *Keystroke::New(1, 0.f),
        *Keystroke::New(3, 0.f),
    };
    REQUIRE(*hotkeys.GetNextEquipset(ks) == "a2");

    ks = std::vector<Keystroke>{
        *Keystroke::New(1, 0.f),
        *Keystroke::New(2, 0.f),
        *Keystroke::New(3, 0.f),
    };
    REQUIRE(*hotkeys.GetNextEquipset(ks) == "a1");
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

    SECTION("infinite hold of earlier hotkey blocks activation of later ones") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "a2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold - .01f),
            *Keystroke::New(3, 0.f),
        };
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(*hotkeys.GetActiveEquipset() == "a2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold),
            *Keystroke::New(3, Keysets::kHoldThreshold - .01f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold),
            *Keystroke::New(3, Keysets::kHoldThreshold),
        };
        // GetNextEquipset() already returned "a1", so now returns nullptr.
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(*hotkeys.GetActiveEquipset() == "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold),
        };
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(*hotkeys.GetActiveEquipset() == "a1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, Keysets::kHoldThreshold),
            *Keystroke::New(3, 0.f),
        };
        REQUIRE(!hotkeys.GetNextEquipset(ks));
        REQUIRE(*hotkeys.GetActiveEquipset() == "a1");
    }

    SECTION("activation of earlier hotkey takes precedence over infinite hold of later ones") {
        auto ks = std::vector<Keystroke>{
            *Keystroke::New(3, 0.f),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b2");

        ks = std::vector<Keystroke>{
            *Keystroke::New(3, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b1");

        ks = std::vector<Keystroke>{
            *Keystroke::New(1, 0.f),
            *Keystroke::New(3, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "a1");

        // Letting go of {1} causes {3} to take over again.
        ks = std::vector<Keystroke>{
            *Keystroke::New(3, Keysets::kHoldThreshold),
        };
        REQUIRE(*hotkeys.GetNextEquipset(ks) == "b1");
    }
}

}  // namespace ech
