cmake_minimum_required(VERSION 3.26.4)

project(NEONCommonTests VERSION 0.0.1 LANGUAGES CXX)

	find_package(GTest CONFIG REQUIRED)

	file(GLOB NEONCommonTests_SourceFiles CONFIGURE_DEPENDS *.cpp *.h)
	
add_executable(NEONCommonTests
	${NEONCommonTests_SourceFiles}
)

target_compile_features(NEONCommonTests PRIVATE cxx_std_23)

target_link_libraries(NEONCommonTests
	PRIVATE
		NEONCommon
		GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
)
