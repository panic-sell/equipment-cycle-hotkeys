vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/CommonLibVR
    REF a5859e42cbd83421527b5816054e346a78d52411
    SHA512 e3e4240a71e1816d9cf3f428b6dfe01c9ce121148fa938ea8e94e5822a8f5e662a6babef49b5c2be972ae7a5753483f1a88a4b801fbc88ed1dc937fb85db2d00
    HEAD_REF ng
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH_OPENVR
    REPO ValveSoftware/openvr
    REF ebdea152f8aac77e9a6db29682b81d762159df7e
    SHA512 4fb668d933ac5b73eb4e97eb29816176e500a4eaebe2480cd0411c95edfb713d58312036f15db50884a2ef5f4ca44859e108dec2b982af9163cefcfc02531f63
    HEAD_REF master
)

file(GLOB OPENVR_FILES "${SOURCE_PATH_OPENVR}/*")
file(COPY ${OPENVR_FILES} DESTINATION "${SOURCE_PATH}/extern/openvr")
file(INSTALL "${SOURCE_PATH_OPENVR}/headers/openvr.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

vcpkg_configure_cmake(
    SOURCE_PATH "${SOURCE_PATH}"
    PREFER_NINJA
    OPTIONS
        -DSKSE_SUPPORT_XBYAK=on
        -DBUILD_TESTS=off
        -DENABLE_SKYRIM_SE=on
        -DENABLE_SKYRIM_AE=on
        -DENABLE_SKYRIM_VR=off
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake)
vcpkg_copy_pdbs()

file(GLOB CMAKE_CONFIGS "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE/*.cmake")
file(INSTALL ${CMAKE_CONFIGS} DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE")
