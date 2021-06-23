# CMakeList.txt : CMake project for influxdb-cpp-2.0, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(project_name "testdemo")
project(${project_name})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/vendor/common
	${CMAKE_SOURCE_DIR}/vendor/zlib-1.2.11
	${CMAKE_BINARY_DIR}/vendor/zlib-1.2.11
)
link_directories(
	${CMAKE_BINARY_DIR}/vendor/zlib-1.2.11
)

list(APPEND LIBS
	zlibstatic
	ws2_32
)

if(MSVC)
	if(CMAKE_CL_64)
		link_directories(
			${CMAKE_SOURCE_DIR}/vendor/Lib/x64
		)
	else(CMAKE_CL_64)
		link_directories(
			${CMAKE_SOURCE_DIR}/vendor/Lib/Win32
		)
	endif(CMAKE_CL_64)
	list(APPEND LIBS ws2_32)
else(MSVC)
	list(APPEND LIBS)
endif(MSVC)

file(GLOB SRCS "*.c" "*.cpp")

# Add source to this project's executable.
add_executable (${project_name} ${SRCS})

target_link_libraries(${project_name} ${LIBS})
# TODO: Add tests and install targets if needed.
