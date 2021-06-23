# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("influxdb-cpp-2.0")

if (MSVC)
	MESSAGE(STATUS "CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
	set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		)
	foreach(CompilerFlag ${CompilerFlags})
		string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
	endforeach()
	add_compile_options("/IGNORE:4200" "/bigobj")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
	add_link_options("/IGNORE:4006,4075,4088,4099")
else(MSVC)
endif(MSVC)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
	add_compile_definitions(BUILD_DEBUG)
else()
	add_compile_definitions()
endif()
if (CMAKE_HOST_APPLE)
	message(STATUS "Platform=APPLE")
	add_compile_definitions(BUILD_PLATFORM_APPLE)
elseif (CMAKE_HOST_UNIX)
	message(STATUS "Platform=LINUX")
	add_compile_definitions(BUILD_PLATFORM_LINUX)
elseif (CMAKE_HOST_WIN32)
	message(STATUS "Platform=WINDOWS")
	add_compile_definitions(BUILD_PLATFORM_WINDOWS)
endif ()

option(ENABLE_GZIP "Option for enable or disable gzip" ON)

message(STATUS "${ENABLE_GZIP}")
# Include sub-lib-projects.
if(ENABLE_GZIP)
add_subdirectory ("vendor/zlib-1.2.11")
add_definitions(-DENABLE_GZIP=1)
endif(ENABLE_GZIP)
# Include sub-projects.
add_subdirectory ("influxdb-cpp-2.0")
