cmake_minimum_required(VERSION 3.28.1)
project(EquipmentCycleHotkeys VERSION 1.0.0 LANGUAGES CXX)


###########################################################
### Files
###########################################################

set(headers
    "src/equipsets.h"
    "src/fs.h"
    "src/gear.h"
    "src/hotkeys.h"
    "src/input_handler.h"
    "src/keys.h"
    "src/serde.h"
    "src/settings.h"
    "src/tes_util.h"
    "src/ui_drawing.h"
    "src/ui_plumbing.h"
    "src/ui_state.h"
)
set(tests
    "tests/equipset_tests.cpp"
    "tests/fs_tests.cpp"
    "tests/hotkey_tests.cpp"
    "tests/key_tests.cpp"
    "tests/serde_tests.cpp"
    "tests/ui_state_tests.cpp"
)


###########################################################
### Plugin Setup
###########################################################

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)  # for alandtse/CommonLibVR
find_package(Boost 1.83.0 REQUIRED COMPONENTS json)
find_package(imgui CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} "src/main.cpp")
target_precompile_headers(${PROJECT_NAME} PUBLIC "src/pch.h")
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${Boost_LIBRARIES}
    d3d11  # for imgui
    imgui::imgui
)

set(DEV_APP_NAME "${PROJECT_NAME}_DevApp")
add_executable(${DEV_APP_NAME} ${headers} "src/main_dev.cpp")
target_link_libraries(${DEV_APP_NAME} PRIVATE ${PROJECT_NAME})


###########################################################
### Test Setup
###########################################################

set(TEST_NAME "${PROJECT_NAME}_Tests")

find_package(Catch2 CONFIG REQUIRED)
add_executable(${TEST_NAME} ${headers} "tests/test_util.h" ${tests})
target_precompile_headers(${TEST_NAME} PRIVATE "tests/pch.h")
target_include_directories(${PROJECT_NAME} PRIVATE "tests")
target_link_libraries(${TEST_NAME} PRIVATE
    ${PROJECT_NAME}
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)

catch_discover_tests(${TEST_NAME})
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})


###########################################################
### DLL Distribution
###########################################################

set(OUTPUT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/misc/SKSE/Plugins")
# Example contents:
# list(APPEND OUTPUT_DIRS "C:/mo2_skyrim_ae/mods/Equipment Cycle Hotkeys/SKSE/Plugins")
# list(APPEND OUTPUT_DIRS "C:/mo2_skyrim_se/mods/Equipment Cycle Hotkeys/SKSE/Plugins")
include(".sandbox/output_dirs.cmake" OPTIONAL)

foreach(DIR ${OUTPUT_DIRS})
    message(STATUS "Output dir: ${DIR}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
endforeach()
