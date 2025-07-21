cmake_minimum_required(VERSION 3.26.4)

project(WiredGPU VERSION 0.0.1 LANGUAGES CXX)

	find_package(glm CONFIG REQUIRED)

	if (WIRED_OPT_IMGUI)
		message("WiredGPU: Configuring for ImGui support")
		find_package(imgui CONFIG REQUIRED)
		find_package(implot CONFIG REQUIRED)
	endif()

	file(GLOB WiredGPU_PublicIncludes CONFIGURE_DEPENDS include/Wired/GPU/*.h)

add_library(WiredGPU INTERFACE
	${WiredGPU_PublicIncludes}
)

if (WIRED_OPT_DEV_BUILD)
	target_compile_definitions(WiredGPU INTERFACE WIRED_DEV_BUILD)
endif()

if (WIRED_OPT_IMGUI)
	target_link_libraries(WiredGPU
		INTERFACE
			imgui::imgui
			implot::implot
	)

	target_compile_definitions(WiredGPU INTERFACE WIRED_IMGUI)
endif()

target_compile_features(WiredGPU
	INTERFACE
		cxx_std_23
)

target_link_libraries(WiredGPU
	INTERFACE
		glm::glm
)

target_include_directories(WiredGPU
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT WiredEngine_Development)
