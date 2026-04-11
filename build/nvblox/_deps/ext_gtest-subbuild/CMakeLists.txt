# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.22.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(ext_gtest-populate NONE)



include(ExternalProject)
ExternalProject_Add(ext_gtest-populate
                     "UPDATE_DISCONNECTED" "False" "OVERRIDE_FIND_PACKAGE" "SYSTEM" "URL" "https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz" "URL_HASH" "MD5=7e11f6cfcf6498324ac82d567dcb891e"
                    SOURCE_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_gtest-src"
                    BINARY_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_gtest-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


