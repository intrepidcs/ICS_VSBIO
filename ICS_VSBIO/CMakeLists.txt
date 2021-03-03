cmake_minimum_required (VERSION 2.6)
project (VSBIO)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)

set(target_dir "bin")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

if( MSVC )
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /EHsc /MP /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	ADD_DEFINITIONS(-DUNICODE -D_UNICODE -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
elseif ( CMAKE_COMPILER_IS_GNUCC )
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive -fexceptions -std=c++11 -Wno-everything")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=gnu99 ")
endif()


set( SOURCES
	./main.cpp
	./VSBIODLL.cpp
	./VSBIO/OFile.cpp
	./VSBIO/VSBIO.cpp
	./VSBIO/MessageTimeDecoderVSB.cpp
)

include_directories(
	.
	./VSBIO
)
	
add_definitions(
	#-DVSBIODLL_EXPORTS
)

if (WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

include_directories(
	.
)


add_executable(VSBIO ${SOURCES})


if(WIN32)
	target_link_libraries(VSBIO)
else()
	target_link_libraries(VSBIO "-static -static-libstdc++")
endif()