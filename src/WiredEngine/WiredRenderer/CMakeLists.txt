cmake_minimum_required(VERSION 3.26.4)

project(WiredRenderer VERSION 0.0.1 LANGUAGES CXX)

	find_package(glm CONFIG REQUIRED)

	file(GLOB WiredRenderer_PublicIncludes CONFIGURE_DEPENDS include/Wired/Render/*.h)
	file(GLOB WiredRenderer_PublicIncludes_Mesh CONFIGURE_DEPENDS include/Wired/Render/Mesh/*.h)
	file(GLOB WiredRenderer_PublicIncludes_Material CONFIGURE_DEPENDS include/Wired/Render/Material/*.h)
	file(GLOB WiredRenderer_PublicIncludes_Renderable CONFIGURE_DEPENDS include/Wired/Render/Renderable/*.h)
	file(GLOB WiredRenderer_PublicIncludes_Task CONFIGURE_DEPENDS include/Wired/Render/Task/*.h)

	file(GLOB WiredRenderer_Sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
	file(GLOB WiredRenderer_Sources_DataStore CONFIGURE_DEPENDS src/DataStore/*.cpp src/DataStore/*.h)
	file(GLOB WiredRenderer_Sources_DrawPass CONFIGURE_DEPENDS src/DrawPass/*.cpp src/DrawPass/*.h)
	file(GLOB WiredRenderer_Sources_Util CONFIGURE_DEPENDS src/Util/*.cpp src/Util/*.h)
	file(GLOB WiredRenderer_Sources_Renderer CONFIGURE_DEPENDS src/Renderer/*.cpp src/Renderer/*.h)

add_library(WiredRenderer
	${WiredRenderer_PublicIncludes}
	${WiredRenderer_PublicIncludes_Mesh}
	${WiredRenderer_PublicIncludes_Material}
	${WiredRenderer_PublicIncludes_Renderable}
	${WiredRenderer_PublicIncludes_Task}

	${WiredRenderer_Sources}
	${WiredRenderer_Sources_DataStore}
	${WiredRenderer_Sources_DrawPass}
	${WiredRenderer_Sources_Util}
	${WiredRenderer_Sources_Renderer}
)

target_link_libraries(WiredRenderer
	PUBLIC
		NEONCommon
		WiredGPU
		glm::glm
)

if (WIRED_OPT_DEV_BUILD)
	target_compile_definitions(WiredRenderer PUBLIC WIRED_DEV_BUILD)
endif()

target_compile_features(WiredRenderer
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(WiredRenderer
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
		${WIRED_LIBRARY_FLAGS}
)

target_include_directories(WiredRenderer
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(WiredRenderer
	PROPERTIES
		OUTPUT_NAME WiredRenderer
		EXPORT_NAME WiredRenderer
		VERSION ${WiredRenderer_VERSION}
		SOVERSION ${WiredRenderer_VERSION_MAJOR}
		INSTALL_RPATH "$ORIGIN"
		INSTALL_RPATH_USE_LINK_PATH TRUE
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
