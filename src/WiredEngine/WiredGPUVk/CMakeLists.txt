cmake_minimum_required(VERSION 3.26.4)

project(WiredGPUVk VERSION 0.0.1 LANGUAGES CXX)

	find_package(VulkanHeaders REQUIRED)
	find_package(VulkanMemoryAllocator CONFIG REQUIRED)
	find_package(unofficial-spirv-reflect CONFIG REQUIRED)

	file(GLOB WiredGPUVk_PublicIncludes CONFIGURE_DEPENDS include/Wired/GPU/*.h)

	file(GLOB WiredGPUVk_SourceFiles CONFIGURE_DEPENDS src/*.cpp src/*.h)
	file(GLOB WiredGPUVk_Vulkan_SourceFiles CONFIGURE_DEPENDS src/Vulkan/*.cpp src/Vulkan/*.h)
	file(GLOB WiredGPUVk_Util_SourceFiles CONFIGURE_DEPENDS src/Util/*.cpp src/Util/*.h)
	file(GLOB WiredGPUVk_State_SourceFiles CONFIGURE_DEPENDS src/State/*.cpp src/State/*.h)
	file(GLOB WiredGPUVk_Frame_SourceFiles CONFIGURE_DEPENDS src/Frame/*.cpp src/Frame/*.h)
	file(GLOB WiredGPUVk_Image_SourceFiles CONFIGURE_DEPENDS src/Image/*.cpp src/Image/*.h)
	file(GLOB WiredGPUVk_Buffer_SourceFiles CONFIGURE_DEPENDS src/Buffer/*.cpp src/Buffer/*.h)
	file(GLOB WiredGPUVk_Shader_SourceFiles CONFIGURE_DEPENDS src/Shader/*.cpp src/Shader/*.h)
	file(GLOB WiredGPUVk_Sampler_SourceFiles CONFIGURE_DEPENDS src/Sampler/*.cpp src/Sampler/*.h)
	file(GLOB WiredGPUVk_Pipeline_SourceFiles CONFIGURE_DEPENDS src/Pipeline/*.cpp src/Pipeline/*.h)
	file(GLOB WiredGPUVk_Descriptor_SourceFiles CONFIGURE_DEPENDS src/Descriptor/*.cpp src/Descriptor/*.h)

add_library(WiredGPUVk
	${WiredGPUVk_PublicIncludes}

	${WiredGPUVk_SourceFiles}
	${WiredGPUVk_Vulkan_SourceFiles}
	${WiredGPUVk_Util_SourceFiles}
	${WiredGPUVk_State_SourceFiles}
	${WiredGPUVk_Frame_SourceFiles}
	${WiredGPUVk_Image_SourceFiles}
	${WiredGPUVk_Buffer_SourceFiles}
	${WiredGPUVk_Shader_SourceFiles}
	${WiredGPUVk_Sampler_SourceFiles}
	${WiredGPUVk_Pipeline_SourceFiles}
	${WiredGPUVk_Descriptor_SourceFiles}
)

target_link_libraries(WiredGPUVk
	PUBLIC
		WiredGPU
		Vulkan::Headers
	PRIVATE
		NEONCommon
		GPUOpen::VulkanMemoryAllocator
		unofficial::spirv-reflect
)

target_compile_features(WiredGPUVk
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(WiredGPUVk
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
)

target_include_directories(WiredGPUVk
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(WiredGPUVk
	PROPERTIES
		OUTPUT_NAME WiredGPUVk
		EXPORT_NAME WiredGPUVk
		VERSION ${WiredGPUVk_VERSION}
		SOVERSION ${WiredGPUVk_VERSION_MAJOR}
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
