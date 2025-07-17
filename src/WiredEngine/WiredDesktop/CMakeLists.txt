cmake_minimum_required(VERSION 3.26.4)

project(WiredDesktop VERSION 0.0.1 LANGUAGES CXX)

	find_package(SDL3 CONFIG REQUIRED)
	find_package(SDL3_ttf CONFIG REQUIRED)

	file(GLOB WiredDesktop_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/*.h)
	file(GLOB WiredDesktop_SourceFiles CONFIGURE_DEPENDS src/*.cpp src/*.h)

add_library(WiredDesktop
	${WiredDesktop_PublicIncludes}
	${WiredDesktop_SourceFiles}
)

target_link_libraries(WiredDesktop
	PUBLIC
		NEONCommon
		WiredEngine
	PRIVATE
		SDL3::SDL3
		$<IF:$<TARGET_EXISTS:SDL3_ttf::SDL3_ttf>,SDL3_ttf::SDL3_ttf,SDL3_ttf::SDL3_ttf-static>
		WiredGPUVk
		WiredPlatformSDL
)

target_compile_features(WiredDesktop
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(WiredDesktop
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
		${WIRED_LIBRARY_FLAGS}
)

target_include_directories(WiredDesktop
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(WiredDesktop
	PROPERTIES
		OUTPUT_NAME WiredDesktop
		EXPORT_NAME WiredDesktop
		VERSION ${WiredDesktop_VERSION}
		SOVERSION ${WiredDesktop_VERSION_MAJOR}
		INSTALL_RPATH "$ORIGIN"
		INSTALL_RPATH_USE_LINK_PATH TRUE
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
