cmake_minimum_required(VERSION 3.28.1)
project(EquipmentCycleHotkeys VERSION 1.2.0 LANGUAGES CXX)


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
set(test_headers
    "tests/test_util.h"
)
set(test_sources
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
find_package(directxtk CONFIG REQUIRED)  # for clib-ng-alt -> alandtse/CommonLibVR
find_package(Boost 1.83.0 REQUIRED COMPONENTS json)
find_package(imgui CONFIG REQUIRED)

add_commonlibsse_plugin("${PROJECT_NAME}" AUTHOR "panic-sell" SOURCES ${headers} "src/main.cpp")
target_precompile_headers("${PROJECT_NAME}" PUBLIC "src/pch.h")
target_include_directories("${PROJECT_NAME}" PUBLIC "src")
target_link_libraries("${PROJECT_NAME}" PUBLIC
    ${Boost_LIBRARIES}
    d3d11  # for imgui
    imgui::imgui
)

set(DEV_APP_NAME "${PROJECT_NAME}_DevApp")
add_executable("${DEV_APP_NAME}" ${headers} "src/main_dev.cpp")
target_link_libraries("${DEV_APP_NAME}" PRIVATE "${PROJECT_NAME}")


###########################################################
### Test Setup
###########################################################

set(TEST_NAME "${PROJECT_NAME}_Tests")

find_package(Catch2 CONFIG REQUIRED)
add_executable("${TEST_NAME}" ${headers} ${test_headers} ${test_sources})
target_precompile_headers("${TEST_NAME}" PRIVATE "tests/pch.h")
target_include_directories("${PROJECT_NAME}" PRIVATE "tests")
target_link_libraries("${TEST_NAME}" PRIVATE
    "${PROJECT_NAME}"
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)

catch_discover_tests("${TEST_NAME}")
add_test(NAME "${TEST_NAME}" COMMAND "${TEST_NAME}")


###########################################################
### DLL Distribution
###########################################################

set(DLL_DEST_DIRS)

# Example contents:
# list(APPEND DLL_DEST_DIRS "C:/mo2_skyrim_ae/mods/EquipmentCycleHotkeys/SKSE/Plugins")
# list(APPEND DLL_DEST_DIRS "C:/mo2_skyrim_se/mods/EquipmentCycleHotkeys/SKSE/Plugins")
include("cmake/dirs.cmake" OPTIONAL)

# Copy DLL to destinations.
foreach(DIR ${DLL_DEST_DIRS})
    message(STATUS "DLL destination: ${DIR}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD

        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIR}"

        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"

        VERBATIM
    )
endforeach()


###########################################################
### Mod Distribution
###########################################################

add_custom_target(mk_dist_dir
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_SOURCE_DIR}/dist"
    VERBATIM
)

# Create mod description for Nexus Mods using https://github.com/ceejbot/md2nexus.
set(MD2NEXUS "${PROJECT_SOURCE_DIR}/.resources/md2nexus.exe")
if(EXISTS "${MD2NEXUS}")
    message(STATUS "md2nexus: ${MD2NEXUS}")

    add_custom_target(md2nexus ALL
        COMMAND "${MD2NEXUS}"
            --input "${PROJECT_SOURCE_DIR}/misc/mod_desc.md"
            --output "${PROJECT_SOURCE_DIR}/dist/mod_desc.bbcode"

        DEPENDS mk_dist_dir
        VERBATIM
    )
endif()

# Create ZIP file for mod.
add_custom_target(zip_mod ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_SOURCE_DIR}/dist/SKSE/Plugins"

    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${PROJECT_SOURCE_DIR}/dist/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"

    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/misc/EquipmentCycleHotkeys.json"
        "${PROJECT_SOURCE_DIR}/dist/SKSE/Plugins/EquipmentCycleHotkeys.json"

    COMMAND "${CMAKE_COMMAND}" -E tar cvf
        "${PROJECT_SOURCE_DIR}/dist/${PROJECT_NAME}-${CMAKE_BUILD_TYPE}-${PROJECT_VERSION}.zip"
        --format=zip -- "${PROJECT_SOURCE_DIR}/dist/SKSE"

    DEPENDS mk_dist_dir "${PROJECT_NAME}"
    # tar is sensitive to the working directory.
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/dist"
    VERBATIM
)
