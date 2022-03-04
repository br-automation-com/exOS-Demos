cmake_minimum_required(VERSION 3.0)

project(simpanel)

set(CMAKE_BUILD_TYPE Debug)

set(SIMPANEL_MODULE_FILES
    l_SimPanel.node
    simpanel.js
    client.js
    package.json
    package-lock.json
    )

install(FILES ${SIMPANEL_MODULE_FILES} DESTINATION /home/user/simpanel)
install(DIRECTORY node_modules DESTINATION /home/user/simpanel)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME exos-comp-simpanel)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SimPanel summary")
set(CPACK_PACKAGE_DESCRIPTION "Some description")
set(CPACK_PACKAGE_VENDOR "Your Company")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME exos-comp-simpanel_1.0.0_amd64)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

