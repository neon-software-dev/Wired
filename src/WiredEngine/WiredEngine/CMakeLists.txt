cmake_minimum_required(VERSION 3.26.4)

project(WiredEngine VERSION 0.0.1 LANGUAGES CXX)

	find_package(EnTT CONFIG REQUIRED)
	find_package(assimp CONFIG REQUIRED)
	find_package(Jolt CONFIG REQUIRED)
	find_package(AudioFile CONFIG REQUIRED)
	find_package(OpenAL CONFIG REQUIRED)
	find_package(nlohmann_json CONFIG REQUIRED)

	file(GLOB WiredEngine_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/*.h)
	file(GLOB WiredEngine_World_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/World/*.h)
	file(GLOB WiredEngine_Package_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Package/*.h)
	file(GLOB WiredEngine_Model_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Model/*.h)
	file(GLOB WiredEngine_Render_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Render/*.h)
	file(GLOB WiredEngine_Physics_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Physics/*.h)
	file(GLOB WiredEngine_Audio_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Audio/*.h)
	file(GLOB WiredEngine_Player_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Player/*.h)
	file(GLOB WiredEngine_Util_PublicIncludes CONFIGURE_DEPENDS include/Wired/Engine/Util/*.h)

	file(GLOB WiredEngine_SourceFiles CONFIGURE_DEPENDS src/*.cpp src/*.h)
	file(GLOB WiredEngine_World_SourceFiles CONFIGURE_DEPENDS src/World/*.cpp src/World/*.h)
	file(GLOB WiredEngine_Package_SourceFiles CONFIGURE_DEPENDS src/Package/*.cpp src/Package/*.h)
	file(GLOB WiredEngine_Model_SourceFiles CONFIGURE_DEPENDS src/Model/*.cpp src/Model/*.h)
	file(GLOB WiredEngine_Physics_SourceFiles CONFIGURE_DEPENDS src/Physics/*.cpp src/Physics/*.h)
	file(GLOB WiredEngine_Audio_SourceFiles CONFIGURE_DEPENDS src/Audio/*.cpp src/Audio/*.h)
	file(GLOB WiredEngine_Player_SourceFiles CONFIGURE_DEPENDS src/Player/*.cpp src/Player/*.h)
	file(GLOB WiredEngine_Util_SourceFiles CONFIGURE_DEPENDS src/Util/*.cpp src/Util/*.h)
	file(GLOB WiredEngine_Font_SourceFiles CONFIGURE_DEPENDS src/Font/*.cpp src/Font/*.h)

add_library(WiredEngine
	${WiredEngine_PublicIncludes}
	${WiredEngine_World_PublicIncludes}
	${WiredEngine_Package_PublicIncludes}
	${WiredEngine_Model_PublicIncludes}
	${WiredEngine_Render_PublicIncludes}
	${WiredEngine_Physics_PublicIncludes}
	${WiredEngine_Audio_PublicIncludes}
	${WiredEngine_Player_PublicIncludes}
	${WiredEngine_Util_PublicIncludes}

	${WiredEngine_SourceFiles}
	${WiredEngine_World_SourceFiles}
	${WiredEngine_Package_SourceFiles}
	${WiredEngine_Model_SourceFiles}
	${WiredEngine_Physics_SourceFiles}
	${WiredEngine_Audio_SourceFiles}
	${WiredEngine_Player_SourceFiles}
	${WiredEngine_Util_SourceFiles}
	${WiredEngine_Font_SourceFiles}
)

target_link_libraries(WiredEngine
	PUBLIC
		NEONCommon
		WiredPlatform
		WiredGPU
	PRIVATE
		EnTT::EnTT
		assimp::assimp
		Jolt::Jolt
		AudioFile::AudioFile
		OpenAL::OpenAL
		nlohmann_json::nlohmann_json
)

target_compile_features(WiredEngine
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(WiredEngine
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
		${WIRED_LIBRARY_FLAGS}
)

target_include_directories(WiredEngine
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(WiredEngine
	PROPERTIES
		OUTPUT_NAME WiredEngine
		EXPORT_NAME WiredEngine
		VERSION ${WiredEngine_VERSION}
		SOVERSION ${WiredEngine_VERSION_MAJOR}
		INSTALL_RPATH "$ORIGIN"
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT WiredEngine_Development)
