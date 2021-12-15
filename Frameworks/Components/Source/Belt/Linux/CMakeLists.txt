cmake_minimum_required(VERSION 3.0)

project(belt)

set(CMAKE_BUILD_TYPE Debug)

set(BELT_MODULE_FILES
    l_Belt.node
    belt.js
    package.json
    package-lock.json
    )

install(FILES ${BELT_MODULE_FILES} DESTINATION /home/user/belt)
install(DIRECTORY node_modules DESTINATION /home/user/belt)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME exos-comp-belt)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Belt summary")
set(CPACK_PACKAGE_DESCRIPTION "Some description")
set(CPACK_PACKAGE_VENDOR "Your Company")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME exos-comp-belt_1.0.0_amd64)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

