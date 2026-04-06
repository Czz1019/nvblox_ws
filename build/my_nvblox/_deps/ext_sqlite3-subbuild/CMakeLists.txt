# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.22.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(ext_sqlite3-populate NONE)



include(ExternalProject)
ExternalProject_Add(ext_sqlite3-populate
                     "UPDATE_DISCONNECTED" "False" "OVERRIDE_FIND_PACKAGE" "SYSTEM" "URL" "https://sqlite.org/2025/sqlite-amalgamation-3500400.zip" "URL_HASH" "MD5=440abd85c5ee3297dd388ade51fec0cc"
                    SOURCE_DIR          "/home/czz/nvblox_ws/build/my_nvblox/_deps/ext_sqlite3-src"
                    BINARY_DIR          "/home/czz/nvblox_ws/build/my_nvblox/_deps/ext_sqlite3-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


