# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.22.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(ext_gflags-populate NONE)



include(ExternalProject)
ExternalProject_Add(ext_gflags-populate
                     "UPDATE_DISCONNECTED" "False" "SYSTEM" "OVERRIDE_FIND_PACKAGE" "URL" "https://github.com/gflags/gflags/archive/refs/tags/v2.2.0.tar.gz" "URL_HASH" "MD5=b99048d9ab82d8c56e876fb1456c285e"
                    SOURCE_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_gflags-src"
                    BINARY_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_gflags-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


