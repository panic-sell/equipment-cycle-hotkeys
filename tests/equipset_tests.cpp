#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "equipsets.h"

namespace ech {
namespace {

using TestEquipsets = Equipsets<int>;

}  // namespace

TEST_CASE("Equipsets empty") {
    auto es = TestEquipsets{};
    REQUIRE(!es.GetActive());
    es.ActivateNext();
    es.ActivateNext();
    REQUIRE(!es.GetActive());
}

TEST_CASE("Equipsets nonempty") {
    struct Testcase {
        std::string_view name;
        TestEquipsets equipsets;
        int initial_slot;
        int next_slot;
        size_t additional_increments;
        int slot_after_increments;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "one_slot",
            .equipsets = TestEquipsets({1}),
            .initial_slot = 1,
            .next_slot = 1,
            .additional_increments = 4,
            .slot_after_increments = 1,
        },
        Testcase{
            .name = "many_slots",
            .equipsets = TestEquipsets({1, 2, 3, 4, 5}),
            .initial_slot = 1,
            .next_slot = 2,
            .additional_increments = 2,
            .slot_after_increments = 4,
        },
        Testcase{
            .name = "many_slots_and_even_more_increments",
            .equipsets = TestEquipsets({1, 2, 3, 4, 5}),
            .initial_slot = 1,
            .next_slot = 2,
            .additional_increments = 103,  // index = (1 + 103) % 5 -> 4
            .slot_after_increments = 5,
        }
    );

    CAPTURE(testcase.name);

    auto& es = testcase.equipsets;
    REQUIRE(*es.GetActive() == testcase.initial_slot);

    es.ActivateNext();
    REQUIRE(*es.GetActive() == testcase.next_slot);

    for (size_t i = 0; i < testcase.additional_increments; i++) {
        es.ActivateNext();
    }
    REQUIRE(*es.GetActive() == testcase.slot_after_increments);

    es.ActivateFirst();
    REQUIRE(*es.GetActive() == testcase.initial_slot);
}

}  // namespace ech
