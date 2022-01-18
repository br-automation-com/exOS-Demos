cmake_minimum_required(VERSION 3.0)

project(sequence)

set(CMAKE_BUILD_TYPE Debug)

set(SEQUENCE_MODULE_FILES
    l_Sequence.node
    sequence.js
    package.json
    package-lock.json
    )

install(FILES ${SEQUENCE_MODULE_FILES} DESTINATION /home/user/sequence)
install(DIRECTORY node_modules DESTINATION /home/user/sequence)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME exos-comp-sequence)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sequence summary")
set(CPACK_PACKAGE_DESCRIPTION "Some description")
set(CPACK_PACKAGE_VENDOR "Your Company")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME exos-comp-sequence_1.0.0_amd64)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

