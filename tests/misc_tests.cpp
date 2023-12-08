#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "tes_util.h"

namespace ech {

TEST_CASE("misc") {
    std::println("{}", std::format("{}", RE::FormTypeToString(RE::FormType::Ammo)));
    std::println("{:08x}", 0xfa123);
    // auto* x = tes_util::GetForm<RE::TESObjectWEAP>(0xff112233);
    REQUIRE(true);
}

}  // namespace ech
