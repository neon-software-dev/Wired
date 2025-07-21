cmake_minimum_required(VERSION 3.26.4)

project(Engine VERSION 0.0.1)

######
# Configuration
######

# When building Wired, add this compile definition so that functions/classes are exported in the resulting binary.
# Client builds won't have this definition set and thus for them function/class signatures will be imported.
add_compile_definitions(NEON_DO_EXPORT)

######
# Subprojects
######

# Subprojects common to all target platforms
add_subdirectory(NEONCommon)
add_subdirectory(WiredRenderer)
add_subdirectory(WiredGPU)
add_subdirectory(WiredGPUVk)
add_subdirectory(WiredPlatform)
add_subdirectory(WiredPlatformSDL)
add_subdirectory(WiredEngine)

# Platform-specific subprojects
if (WIREDENGINE_TARGET_PLATFORM STREQUAL ${WIREDENGINE_PLATFORM_DESKTOP})
    message("WiredEngine: Configuring for desktop platform")
    add_subdirectory(WiredDesktop)
    add_subdirectory(NEONCommonTests)
elseif (WIREDENGINE_TARGET_PLATFORM STREQUAL ${WIREDENGINE_PLATFORM_ANDROID})
    message("WiredEngine: Configuring for android platform")
    #add_subdirectory(WiredAndroid)
endif()

####
# Installation
####

if (WIREDENGINE_TARGET_PLATFORM STREQUAL ${WIREDENGINE_PLATFORM_DESKTOP})

    set(WIREDENGINE_NAMESPACE            "Wired::")
    set(WIREDENGINE_PACKAGENAME          "WiredEngine")
    set(WIREDENGINE_GENERATED_DIR        "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(WIREDENGINE_VERSION_CONFIG_FILE  "${WIREDENGINE_GENERATED_DIR}/${WIREDENGINE_PACKAGENAME}ConfigVersion.cmake")
    set(WIREDENGINE_PROJECT_CONFIG_FILE  "${WIREDENGINE_GENERATED_DIR}/${WIREDENGINE_PACKAGENAME}Config.cmake")
    set(WIREDENGINE_CONFIG_INSTALL_DIR   "cmake/${WIREDENGINE_PACKAGENAME}")
    set(WIREDENGINE_TARGETS_EXPORT_NAME  "${WIREDENGINE_PACKAGENAME}Targets")

    install(
        TARGETS
            NEONCommon
            WiredRenderer
            WiredGPU
            WiredGPUVk
            WiredPlatform
            WiredPlatformSDL
            WiredEngine
            WiredDesktop
        EXPORT ${WIREDENGINE_TARGETS_EXPORT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT WiredEngine_Runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT WiredEngine_Runtime NAMELINK_COMPONENT WiredEngine_Development
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT WiredEngine_Development
    )

    # Generate and install cmake config files
    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        "${WIREDENGINE_VERSION_CONFIG_FILE}"
        VERSION ${CMAKE_PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in" "${WIREDENGINE_PROJECT_CONFIG_FILE}" @ONLY)

    install(
        FILES "${WIREDENGINE_VERSION_CONFIG_FILE}" "${WIREDENGINE_PROJECT_CONFIG_FILE}"
        DESTINATION "${WIREDENGINE_CONFIG_INSTALL_DIR}"
        COMPONENT WiredEngine_Development
    )

    install(
        EXPORT "${WIREDENGINE_TARGETS_EXPORT_NAME}"
        NAMESPACE "${WIREDENGINE_NAMESPACE}"
        DESTINATION "${WIREDENGINE_CONFIG_INSTALL_DIR}"
        COMPONENT WiredEngine_Development
    )

endif()
