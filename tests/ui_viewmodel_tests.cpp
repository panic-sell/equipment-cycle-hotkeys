#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "hotkeys.h"
#include "ui_viewmodels.h"

namespace ech {
namespace ui {

using TestHotkeysVM = HotkeysVM<std::string_view>;

template <typename T>
requires(std::equality_comparable<T>)
bool
operator==(const HotkeyVM<T>& a, const HotkeyVM<T>& b) {
    return a.name == b.name && a.keysets == b.keysets && a.equipsets == b.equipsets;
}

TEST_CASE("HotkeysVM conversions") {
    auto initial_vm = TestHotkeysVM{
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
    auto want_final_vm = TestHotkeysVM{
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

    auto f = [](const std::string_view& s) { return s; };
    auto hotkeys = initial_vm.To<std::string_view>(f);
    auto got_final_vm = TestHotkeysVM::From<std::string_view>(hotkeys, f);
    REQUIRE(got_final_vm == want_final_vm);
}

}  // namespace ui
}  // namespace ech
