set (cmake-minimum-required 3.18)
cmake_minimum_required(VERSION ${cmake-minimum-required})

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON )

option(CMAKE_EXPORT_COMPILE_COMMANDS   "Export compile-commands"      ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project("IEC104Application" 
	VERSION 0.1.1
	DESCRIPTION "An implementation of IEC60870-5-104"
	LANGUAGES C CXX)

add_subdirectory(3rd)
# Include sub-projects.
add_subdirectory("sources")