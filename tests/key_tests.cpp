#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "keys.h"

namespace ech {

TEST_CASE("Keycode from name") {
    struct Testcase {
        std::string_view name;
        uint32_t want;
    };

    auto testcase = GENERATE(
        Testcase{.name = "", .want = 0},
        Testcase{.name = "081i3nof09", .want = 0},
        Testcase{.name = "LShift", .want = 42},
        Testcase{.name = "lshift", .want = 0},
        Testcase{.name = "GamepadRT", .want = 281},
        Testcase{.name = "GamEPADrt", .want = 0}
    );

    auto got = KeycodeFromName(testcase.name);
    REQUIRE(got == testcase.want);
}

TEST_CASE("Keystroke ctor") {
    struct Testcase {
        std::string_view name;
        uint32_t keycode;
        float heldsecs;
        bool want_null;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "normal",
            .keycode = 1,
            .heldsecs = 0.f,
            .want_null = false,
        },
        Testcase{
            .name = "bad_keycode",
            .keycode = 0,
            .heldsecs = 0.f,
            .want_null = true,
        },
        Testcase{
            .name = "bad_heldsecs_negative",
            .keycode = 1,
            .heldsecs = -.1f,
            .want_null = true,
        },
        Testcase{
            .name = "bad_heldsecs_inf",
            .keycode = 0,
            .heldsecs = std::numeric_limits<float>::infinity(),
            .want_null = true,
        },
        Testcase{
            .name = "bad_heldsecs_nan",
            .keycode = 0,
            .heldsecs = std::numeric_limits<float>::quiet_NaN(),
            .want_null = true,
        }
    );

    auto ks = Keystroke::New(testcase.keycode, testcase.heldsecs);
    CAPTURE(testcase.name);
    if (testcase.want_null) {
        REQUIRE(!ks);
    } else {
        REQUIRE(ks->keycode() == testcase.keycode);
        REQUIRE(ks->heldsecs() == testcase.heldsecs);
    }
}

TEST_CASE("Keysets ctor") {
    struct Testcase {
        std::string_view name;
        std::vector<Keyset> input;
        std::vector<Keyset> want_inner;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "normal",
            .input{
                {0, 0, 0, 0},
                {4, 0, 3, 0},
                {0, 1, 2, 0},
                {0, 0, 0, 0},
                {1, 1, 1, 1},  // duplicates will be removed
                {1, 2, 3, 4},
            },
            .want_inner{
                {3, 4, 0, 0},
                {1, 2, 0, 0},
                {1, 0, 0, 0},
                {1, 2, 3, 4},
            },
        },
        Testcase{
            .name = "empty",
            .input = {},
            .want_inner = {},
        }
    );

    auto ks = Keysets(std::move(testcase.input));
    auto got_inner = ks.vec();
    auto want_inner = std::span<const Keyset>(testcase.want_inner);
    CAPTURE(testcase.name, want_inner, got_inner);
    REQUIRE(std::equal(got_inner.cbegin(), got_inner.cend(), want_inner.cbegin(), want_inner.cend())
    );
}

TEST_CASE("Keysets match") {
    struct Testcase {
        std::string_view name;
        Keysets::MatchResult want;
        Keysets keysets;
        std::vector<Keystroke> keystrokes;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "both_empty",
            .want = Keysets::MatchResult::kNone,
            .keysets = {},
            .keystrokes = {},
        },
        Testcase{
            .name = "empty_keystrokes",
            .want = Keysets::MatchResult::kNone,
            .keysets = Keysets({{1}}),
            .keystrokes = {},
        },
        Testcase{
            .name = "empty_keyset",
            .want = Keysets::MatchResult::kNone,
            .keysets = {},
            .keystrokes{
                *Keystroke::New(1, 0.f),
            },
        },
        Testcase{
            .name = "keyset_not_subset_of_keystrokes",
            .want = Keysets::MatchResult::kNone,
            .keysets = Keysets({
                {1, 2},
                {4},
            }),
            .keystrokes{
                *Keystroke::New(2, 0.f),
                *Keystroke::New(3, 0.f),
            },
        },
        Testcase{
            .name = "match_hold",
            .want = Keysets::MatchResult::kHold,
            .keysets = Keysets({
                {2, 3, 4},
            }),
            .keystrokes{
                *Keystroke::New(1, Keysets::kHoldThreshold),
                *Keystroke::New(2, Keysets::kHoldThreshold),
                *Keystroke::New(3, Keysets::kHoldThreshold),
                *Keystroke::New(4, Keysets::kHoldThreshold),
                *Keystroke::New(5, 0.f),
            },
        },
        Testcase{
            .name = "match_semihold",
            .want = Keysets::MatchResult::kSemihold,
            .keysets = Keysets({
                {2, 3, 4},
            }),
            .keystrokes{
                *Keystroke::New(1, Keysets::kHoldThreshold),
                *Keystroke::New(2, Keysets::kHoldThreshold),
                *Keystroke::New(3, Keysets::kHoldThreshold),
                *Keystroke::New(4, Keysets::kHoldThreshold - .01f),
                *Keystroke::New(5, 0.f),
            },
        },
        Testcase{
            .name = "match_press",
            .want = Keysets::MatchResult::kPress,
            .keysets = Keysets({
                {2, 3, 4},
            }),
            .keystrokes{
                *Keystroke::New(1, Keysets::kHoldThreshold),
                *Keystroke::New(2, 0.f),
                *Keystroke::New(3, Keysets::kHoldThreshold),
                *Keystroke::New(4, Keysets::kHoldThreshold - .01f),
                *Keystroke::New(5, 0.f),
            },
        },
        Testcase{
            .name = "match_multiple_keyset_candidates",
            .want = Keysets::MatchResult::kPress,
            .keysets = Keysets({
                {6},
                {4},  // this gets matched since it precedes "1"
                {1},
            }),
            .keystrokes{
                *Keystroke::New(1, Keysets::kHoldThreshold),
                *Keystroke::New(2, 3.f),
                *Keystroke::New(3, 2.f),
                *Keystroke::New(4, 0.f),
                *Keystroke::New(5, 1.f),
            },
        }
    );

    CAPTURE(testcase.name);
    auto got = testcase.keysets.Match(testcase.keystrokes);
    REQUIRE(got == testcase.want);
}

}  // namespace ech
