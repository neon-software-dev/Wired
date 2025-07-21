cmake_minimum_required(VERSION 3.26.4)

project(Wired VERSION 0.0.1)

####
# Options
####

option(WIRED_OPT_DEV_BUILD "Configure Wired for developer mode" OFF)
option(WIRED_OPT_IMGUI "Support for ImGui rendering" OFF)

######
# Global variables
######

set(WIREDENGINE_PLATFORM_DESKTOP "Desktop")
set(WIREDENGINE_PLATFORM_ANDROID "Android")

# Default target platform to desktop, if not specified
if (NOT DEFINED WIREDENGINE_TARGET_PLATFORM)
    set(WIREDENGINE_TARGET_PLATFORM ${WIREDENGINE_PLATFORM_DESKTOP})
endif()

# On linux, append explicitly dynamically built vcpkg libraries to the prefix path so they can be found
if (NOT WIN32)
    set(VCPKG_SHARED_LIBS_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/vcpkg/installed/x64-linux-dynamic")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_SHARED_LIBS_INSTALLED_DIR}/share/")
endif()

####
# Global variables
####

if (WIRED_OPT_DEV_BUILD)
    message("Wired: Configuring for developer mode")

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message("Wired: Configuring for GCC")
        set(WIRED_WARNINGS_FLAGS
            -Wall -Wextra -Werror -Wpedantic -Wcast-align -Wformat -Wformat=2 -Wlogical-op
            -Wmissing-include-dirs -Wpointer-arith -Wredundant-decls -Wsequence-point
            -Wshadow -Wswitch -Wunreachable-code -Wunused-but-set-parameter -Wfloat-equal
            -Wconversion -Wimplicit-fallthrough -Werror=format-security -Wtrampolines
            -Wbidi-chars=any
            -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
            -D_GLIBCXX_ASSERTIONS
            -fstrict-flex-arrays=3
            -fstack-clash-protection -fstack-protector-strong
            -Wl,-z,nodlopen -Wl,-z,noexecstack
            -Wl,-z,relro -Wl,-z,now
            -Wl,--as-needed -Wl,--no-copy-dt-needed-entries
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message("Wired: Configuring for Clang")
        # TODO: Test building with clang and add in all the clang error flags
        set(WIRED_WARNINGS_FLAGS
            -Wall -Wextra -Werror
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        message("Wired: Configuring for MSVC")
        set(WIRED_WARNINGS_FLAGS "")
    endif()
else()
    message("Wired: Configuring for non-developer mode")

    set(WIRED_WARNINGS_FLAGS "")
endif()

# Flags for all libraries to add to their compile options
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(WIRED_LIBRARY_FLAGS
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )
else()
    set(WIRED_LIBRARY_FLAGS "")
endif()

####
# Add manual external subprojects
####

# no-op

####
# Add subprojects
####

add_subdirectory(WiredEngine)

if (WIREDENGINE_TARGET_PLATFORM STREQUAL ${WIREDENGINE_PLATFORM_DESKTOP})
    if (${WITH_TESTDESKTOPAPP})
        add_subdirectory(TestSuite)
        add_subdirectory(TestDesktopApp)
        add_subdirectory(WiredEditor)
    endif()
endif()
