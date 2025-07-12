cmake_minimum_required(VERSION 3.26.4)

project(WiredPlatform VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB WiredPlatform_PublicIncludes CONFIGURE_DEPENDS include/Wired/Platform/*.h)
	file(GLOB WiredPlatform_Event_PublicIncludes CONFIGURE_DEPENDS include/Wired/Platform/Event/*.h)

add_library(WiredPlatform INTERFACE
	${WiredPlatform_PublicIncludes}
	${WiredPlatform_Event_PublicIncludes}
)

target_link_libraries(WiredPlatform
	INTERFACE
		WiredRenderer
)

target_include_directories(WiredPlatform
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
