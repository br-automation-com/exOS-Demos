cmake_minimum_required(VERSION 3.0)

project(remoteio)

set(CMAKE_BUILD_TYPE Debug)

set(REMOTEIO_MODULE_FILES
    l_RemoteIO.node
    remoteio.js
    package.json
    package-lock.json
    )

install(FILES ${REMOTEIO_MODULE_FILES} DESTINATION /home/user/remoteio)
install(DIRECTORY node_modules DESTINATION /home/user/remoteio)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME exos-comp-remoteio)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RemoteIO summary")
set(CPACK_PACKAGE_DESCRIPTION "Some description")
set(CPACK_PACKAGE_VENDOR "Your Company")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME exos-comp-remoteio_1.0.0_amd64)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

