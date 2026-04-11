# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.22.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(ext_benchmark-populate NONE)



include(ExternalProject)
ExternalProject_Add(ext_benchmark-populate
                     "UPDATE_DISCONNECTED" "False" "OVERRIDE_FIND_PACKAGE" "SYSTEM" "URL" "https://github.com/google/benchmark/archive/refs/tags/v1.9.0.tar.gz" "URL_HASH" "MD5=21a2604efeded8b4cbabc72f3e1c7a2a"
                    SOURCE_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_benchmark-src"
                    BINARY_DIR          "/home/czz/nvblox_ws/build/nvblox/_deps/ext_benchmark-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


