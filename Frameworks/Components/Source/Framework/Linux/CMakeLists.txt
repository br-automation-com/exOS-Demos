cmake_minimum_required(VERSION 3.0)

project(framework)

set(CMAKE_BUILD_TYPE Debug)

set(FRAMEWORK_MODULE_FILES
    framework.js
    test.js
    package.json
    package-lock.json
    )

install(FILES ${FRAMEWORK_MODULE_FILES} DESTINATION /home/user/framework)
install(DIRECTORY node_modules DESTINATION /home/user/framework)
install(DIRECTORY www DESTINATION /home/user/framework)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME exos-comp-framework)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Framework summary")
set(CPACK_PACKAGE_DESCRIPTION "Some description")
set(CPACK_PACKAGE_VENDOR "Your Company")

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME exos-comp-framework_1.0.0_amd64)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Your Name")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

