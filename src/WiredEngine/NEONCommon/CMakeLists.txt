cmake_minimum_required(VERSION 3.26.4)

project(NEONCommon VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB NEONCommon_PublicIncludes CONFIGURE_DEPENDS include/NEON/Common/*.h)
	file(GLOB NEONCommon_PublicIncludes_Container CONFIGURE_DEPENDS include/NEON/Common/Container/*.h)
	file(GLOB NEONCommon_PublicIncludes_Log CONFIGURE_DEPENDS include/NEON/Common/Log/*.h)
	file(GLOB NEONCommon_PublicIncludes_Metrics CONFIGURE_DEPENDS include/NEON/Common/Metrics/*.h)
	file(GLOB NEONCommon_PublicIncludes_Thread CONFIGURE_DEPENDS include/NEON/Common/Thread/*.h)
	file(GLOB NEONCommon_PublicIncludes_Space CONFIGURE_DEPENDS include/NEON/Common/Space/*.h)

	file(GLOB NEONCommon_SourceFiles CONFIGURE_DEPENDS src/*.cpp src/*.h)
	file(GLOB NEONCommon_SourceFiles_Log CONFIGURE_DEPENDS src/Log/*.cpp src/Log/*.h)
	file(GLOB NEONCommon_SourceFiles_Metrics CONFIGURE_DEPENDS src/Metrics/*.cpp src/Metrics/*.h)
	file(GLOB NEONCommon_SourceFiles_Thread CONFIGURE_DEPENDS src/Thread/*.cpp src/Thread/*.h)
	file(GLOB NEONCommon_SourceFiles_Space CONFIGURE_DEPENDS src/Space/*.cpp src/Space/*.h)

add_library(NEONCommon
	${NEONCommon_PublicIncludes}
	${NEONCommon_PublicIncludes_Container}
	${NEONCommon_PublicIncludes_Log}
	${NEONCommon_PublicIncludes_Metrics}
	${NEONCommon_PublicIncludes_Thread}
	${NEONCommon_PublicIncludes_Space}

	${NEONCommon_SourceFiles}
	${NEONCommon_SourceFiles_Log}
	${NEONCommon_SourceFiles_Metrics}
	${NEONCommon_SourceFiles_Thread}
	${NEONCommon_SourceFiles_Space}
)

if (${BUILD_SHARED_LIBS})
	target_compile_definitions(NEONCommon PUBLIC NEON_SHARED)
else()
	target_compile_definitions(NEONCommon PUBLIC NEON_STATIC)
endif()

target_compile_features(NEONCommon
	PUBLIC
		cxx_std_23
	PRIVATE
		cxx_std_23
)

target_compile_options(NEONCommon
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
		${WIRED_LIBRARY_FLAGS}
)

target_include_directories(NEONCommon
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(NEONCommon
	PROPERTIES
		OUTPUT_NAME NEONCommon
		EXPORT_NAME NEONCommon
		VERSION ${NEONCommon_VERSION}
		SOVERSION ${NEONCommon_VERSION_MAJOR}
)

####
# Installation
####

include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
