#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "equipsets.h"
#include "gear.h"

namespace Catch {

template <>
struct StringMaker<ech::GearOrSlot> {
    static std::string
    convert(const ech::GearOrSlot& value) {
        return fmt::format(
            "({}{}{})", value.slot(), value.gear() ? ", " : "", value.gear() ? "gear" : ""
        );
    }
};

}  // namespace Catch

namespace ech {
namespace internal {

Gear
TestGear(Gearslot slot) {
    return Gear(nullptr, slot, 0.f, nullptr);
}

}  // namespace internal

using TestEquipsets = Equipsets<int>;

bool
operator==(const Gear& a, const Gear& b) {
    return a.slot() == b.slot();
}

bool
operator==(const GearOrSlot& a, const GearOrSlot& b) {
    auto* ag = a.gear();
    auto* bg = b.gear();
    if (ag && bg) {
        return *ag == *bg;
    }
    if (!ag && !bg) {
        return a.slot() == b.slot();
    }
    return false;
}

bool
operator==(const Equipset& a, const Equipset& b) {
    return a.vec() == b.vec();
}

TEST_CASE("Equipset ctor") {
    struct Testcase {
        std::string_view name;
        std::vector<GearOrSlot> arg;
        std::vector<GearOrSlot> want;
    };

    auto testcase = GENERATE(
        Testcase{
            .name = "empty",
            .arg{},
            .want{},
        },
        Testcase{
            .name = "ordering",
            .arg{
                internal::TestGear(Gearslot::kShout),
                internal::TestGear(Gearslot::kRight),
                Gearslot::kLeft,
                Gearslot::kAmmo,
            },
            .want{
                Gearslot::kLeft,
                internal::TestGear(Gearslot::kRight),
                internal::TestGear(Gearslot::kShout),
                Gearslot::kAmmo,
            },
        },
        Testcase{
            .name = "remove_duplicates",
            .arg{
                internal::TestGear(Gearslot::kShout),
                Gearslot::kRight,
                Gearslot::kLeft,
                internal::TestGear(Gearslot::kRight),
                internal::TestGear(Gearslot::kLeft),
            },
            .want{
                Gearslot::kLeft,
                internal::TestGear(Gearslot::kShout),
                Gearslot::kRight,
            },
        }
    );

    CAPTURE(testcase.name);
    auto es = Equipset(testcase.arg);
    auto& got = es.vec();
    REQUIRE(got == testcase.want);
}

TEST_CASE("Equipsets empty") {
    auto es = TestEquipsets{};
    REQUIRE(!es.GetActive());
    es.ActivateNext();
    es.ActivateNext();
    REQUIRE(!es.GetActive());
}

TEST_CASE("Equipsets nonempty activation") {
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

TEST_CASE("Equipsets ctor specialization prunes empty equipsets") {
    auto initial = std::vector<Equipset>{
        Equipset(),
        Equipset({Gearslot::kLeft}),
        Equipset(),
        Equipset({internal::TestGear(Gearslot::kLeft)}),
    };
    auto want = std::vector<Equipset>{
        Equipset({Gearslot::kLeft}),
        Equipset({internal::TestGear(Gearslot::kLeft)}),
    };
    auto equipsets = Equipsets(initial);
    REQUIRE(equipsets.vec() == want);
}

}  // namespace ech
