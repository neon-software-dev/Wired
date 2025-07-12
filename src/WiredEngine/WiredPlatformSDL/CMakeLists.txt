cmake_minimum_required(VERSION 3.26.4)

project(WiredPlatformSDL VERSION 0.0.1 LANGUAGES CXX)

	find_package(SDL3 CONFIG REQUIRED)
	find_package(SDL3_image CONFIG REQUIRED)

	file(GLOB WiredPlatformSDL_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/*.h)
	file(GLOB WiredPlatformSDL_Platform_PublicIncludes CONFIGURE_DEPENDS include/Wired/Platform/*.h)
	file(GLOB WiredPlatformSDL_SourceFiles CONFIGURE_DEPENDS src/*.cpp src/*.h)

add_library(WiredPlatformSDL
	${WiredPlatformSDL_PublicIncludes}
	${WiredPlatformSDL_Platform_PublicIncludes}
	${WiredPlatformSDL_SourceFiles}
)

target_link_libraries(WiredPlatformSDL
	PUBLIC
		WiredPlatform
	PRIVATE
		NEONCommon
		SDL3::SDL3
		$<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
)

target_compile_features(WiredPlatformSDL
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(WiredPlatformSDL
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
)

target_include_directories(WiredPlatformSDL
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(WiredPlatformSDL
	PROPERTIES
		OUTPUT_NAME WiredPlatformSDL
		EXPORT_NAME WiredPlatformSDL
		VERSION ${WiredPlatformSDL_VERSION}
		SOVERSION ${WiredPlatformSDL_VERSION_MAJOR}
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
