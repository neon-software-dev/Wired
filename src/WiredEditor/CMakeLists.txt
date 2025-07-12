cmake_minimum_required(VERSION 3.26.4)

project(WiredEditor VERSION 0.0.1 LANGUAGES CXX)

	file(GLOB WiredEditor_SourceFiles CONFIGURE_DEPENDS *.cpp *.h)
	file(GLOB WiredEditor_Window_SourceFiles CONFIGURE_DEPENDS Window/*.cpp Window/*.h)
	file(GLOB WiredEditor_View_SourceFiles CONFIGURE_DEPENDS View/*.cpp View/*.h)
	file(GLOB WiredEditor_ViewModel_SourceFiles CONFIGURE_DEPENDS ViewModel/*.cpp ViewModel/*.h)
	file(GLOB WiredEditor_PopUp_SourceFiles CONFIGURE_DEPENDS PopUp/*.cpp PopUp/*.h)
	file(GLOB WiredEditor_Package_SourceFiles CONFIGURE_DEPENDS Package/*.cpp Package/*.h)

add_executable(WiredEditor
	${WiredEditor_SourceFiles}
	${WiredEditor_Window_SourceFiles}
	${WiredEditor_View_SourceFiles}
	${WiredEditor_ViewModel_SourceFiles}
	${WiredEditor_PopUp_SourceFiles}
	${WiredEditor_Package_SourceFiles}
)

target_compile_options(WiredEditor
	PRIVATE
		${WIRED_WARNINGS_FLAGS}
)

target_compile_features(WiredEditor PRIVATE cxx_std_23)

target_link_libraries(WiredEditor
	PRIVATE
		WiredDesktop
)

# On Windows, copy runtime dlls to same directory as the binary
# From: https://www.youtube.com/watch?v=bemvHlcz1Qc
if (CMAKE_IMPORT_LIBRARY_SUFFIX)
	add_custom_command(
		TARGET WiredEditor POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			-t $<TARGET_FILE_DIR:WiredEditor>
			$<TARGET_RUNTIME_DLLS:WiredEditor>
		COMMAND_EXPAND_LISTS
	)
endif()

# Copy the WiredEditor package to the build output directory
add_custom_target(CopyWiredDirEditor ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/wired/"
		"$<TARGET_FILE_DIR:WiredEditor>/wired/"
		COMMENT "Copying wired directory to runtime output directory"
)
add_dependencies(CopyWiredDirEditor WiredEditor)

# Copy the engine default shaders to the build output directory
add_custom_target(CopyDefaultShadersEditor ALL
		COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/../default_shaders/"
		"$<TARGET_FILE_DIR:WiredEditor>/wired/shaders"
		COMMENT "Copying default shaders to runtime output directory"
)
add_dependencies(CopyDefaultShadersEditor WiredEditor)