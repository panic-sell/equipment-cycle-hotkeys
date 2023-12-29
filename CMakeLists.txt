cmake_minimum_required(VERSION 3.27.2)
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
    "src/fs.h"
    "src/gear.h"
    "src/hotkeys.h"
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
find_package(Boost 1.83.0 REQUIRED COMPONENTS json)
find_package(imgui CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${Boost_LIBRARIES}
    d3d11  # for Dear ImGui
    imgui::imgui
)
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_precompile_headers(${PROJECT_NAME} PUBLIC "src/pch.h")

set(DEV_APP_NAME "${PROJECT_NAME}_DevApp")
add_executable(${DEV_APP_NAME} ${headers} "src/main_dev.cpp")
target_link_libraries(${DEV_APP_NAME} PRIVATE ${PROJECT_NAME})


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
include("misc/dll_folder.cmake" OPTIONAL)

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
