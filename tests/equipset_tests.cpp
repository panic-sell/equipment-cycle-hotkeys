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

using TestEquipsets = Equipsets<int>;

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
                Gear::NewForTest(Gearslot::kShout),
                Gear::NewForTest(Gearslot::kRight),
                Gearslot::kLeft,
                Gearslot::kAmmo,
            },
            .want{
                Gearslot::kLeft,
                Gear::NewForTest(Gearslot::kRight),
                Gear::NewForTest(Gearslot::kShout),
                Gearslot::kAmmo,
            },
        },
        Testcase{
            .name = "remove_duplicates",
            .arg{
                Gear::NewForTest(Gearslot::kShout),
                Gearslot::kRight,
                Gearslot::kLeft,
                Gear::NewForTest(Gearslot::kRight),
                Gear::NewForTest(Gearslot::kLeft),
            },
            .want{
                Gearslot::kLeft,
                Gear::NewForTest(Gearslot::kShout),
                Gearslot::kRight,
            },
        }
    );

    CAPTURE(testcase.name);
    auto es = Equipset(testcase.arg);
    const auto& got = es.vec();
    REQUIRE(got == testcase.want);
}

TEST_CASE("Equipsets empty") {
    auto es = TestEquipsets();
    REQUIRE(!es.GetSelected());
    es.SelectNext();
    es.SelectNext();
    REQUIRE(!es.GetSelected());
}

TEST_CASE("Equipsets nonempty selection") {
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
    REQUIRE(*es.GetSelected() == testcase.initial_slot);

    es.SelectNext();
    REQUIRE(*es.GetSelected() == testcase.next_slot);

    for (size_t i = 0; i < testcase.additional_increments; i++) {
        es.SelectNext();
    }
    REQUIRE(*es.GetSelected() == testcase.slot_after_increments);

    es.SelectFirst();
    REQUIRE(*es.GetSelected() == testcase.initial_slot);
}

TEST_CASE("Equipsets ctor specialization prunes empty equipsets") {
    auto initial = std::vector<Equipset>{
        Equipset(),
        Equipset({Gearslot::kLeft}),
        Equipset(),
        Equipset({Gear::NewForTest(Gearslot::kLeft)}),
    };
    auto want = std::vector<Equipset>{
        Equipset({Gearslot::kLeft}),
        Equipset({Gear::NewForTest(Gearslot::kLeft)}),
    };
    auto equipsets = Equipsets(initial);
    REQUIRE(equipsets.vec() == want);
}

}  // namespace ech
