cmake_minimum_required(VERSION 3.21)
project(EquipmentCycleHotkeys VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)


###########################################################
### Files
###########################################################

set(headers
    "src/dev_util.h"
    "src/equipsets.h"
    "src/event_handler.h"
    "src/gear.h"
    "src/hotkeys.h"
    "src/keys.h"
    "src/tes_util.h"
    "src/ui.h"
    "src/ui_plumbing.h"
    "src/ui_viewmodels.h"
)
set(tests
    "tests/equipset_tests.cpp"
    "tests/hotkey_tests.cpp"
    "tests/key_tests.cpp"
    "tests/misc_tests.cpp"
    "tests/ui_viewmodel_tests.cpp"
)


###########################################################
### Plugin Setup
###########################################################

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC
    d3d11
    imgui::imgui
    nlohmann_json::nlohmann_json
)
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_precompile_headers(${PROJECT_NAME} PUBLIC "src/pch.h")

set(IMGUI_DEMO_NAME "${PROJECT_NAME}_ImguiDemo")
add_executable(${IMGUI_DEMO_NAME} ${headers} "src/main_imgui.cpp")
target_link_libraries(${IMGUI_DEMO_NAME} PRIVATE ${PROJECT_NAME})


###########################################################
### Test Setup
###########################################################

set(TEST_NAME "${PROJECT_NAME}_Tests")

find_package(Catch2 CONFIG REQUIRED)
add_executable(${TEST_NAME} ${headers} ${tests})
target_link_libraries(${TEST_NAME} PRIVATE
    ${PROJECT_NAME}
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)

catch_discover_tests(${TEST_NAME})
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})


###########################################################
### Copy DLL to Skyrim
###########################################################

# Example contents:
# set(DLL_FOLDER "C:/mo2_skyrimse/mods/Equipment Cycle Hotkeys/SKSE/Plugins")
include("dll_folder.cmake" OPTIONAL)

if(DEFINED DLL_FOLDER)
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
endif()
