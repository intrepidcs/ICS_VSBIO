cmake_minimum_required (VERSION 3.15)
project (VSBIO)

include(FetchContent)
FetchContent_Declare(
	pugixml
	GIT_REPOSITORY https://github.com/zeux/pugixml.git
	GIT_TAG        v1.14
)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)

set(target_dir "bin")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_ENABLE_COLUMN_METADATA")

if( MSVC )
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /EHsc /MP /W4")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	ADD_DEFINITIONS(-DUNICODE -D_UNICODE -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
elseif ( CMAKE_COMPILER_IS_GNUCC )
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fpermissive -fexceptions -std=c++11 -Wno-everything" -O3)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=gnu99 -DSQLITE_ENABLE_COLUMN_METADATA" -O3)
if (WIN32)
	ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
endif()
endif()

FetchContent_MakeAvailable(pugixml)


set( SOURCES
	./VSBIODLL.cpp
	./CMPNetworkMap.cpp
	./UnwrapCMP.cpp
	./VSBIO/OFile.cpp
	./VSBIO/VSBIO.cpp
	./VSBIO/MessageTimeDecoderVSB.cpp
	./VSBIO/VSBDatabase.cpp
	./VSBIO/NetworkInfo.cpp
	./VSBIO/PcapFile.cpp
	./VSBIO/sqlite/sqlite3.c
	./VSBIO/sqlite/KompexSQLiteDatabase.cpp
	./VSBIO/sqlite/KompexSQLiteStatement.cpp
	./VSBIO/sqlite/KompexSQLiteBlob.cpp
)

include_directories(
	.
	./VSBIO
	./VSBIO/sqlite
)

#Create test command line program
add_executable(VSBIOTest ./main.cpp ./VSBIO/OFile.cpp)
target_include_directories(VSBIOTest PUBLIC include)
target_link_libraries(VSBIOTest VSBIO)
	
add_definitions(
	-DVSBIODLL_EXPORTS
)

if (WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

include_directories(
	.
)

add_library(VSBIO SHARED ${SOURCES})
target_include_directories(VSBIO PUBLIC include)
target_link_libraries(VSBIO PRIVATE pugixml::pugixml)
if(NOT WIN32)
	target_link_libraries(VSBIO PRIVATE "-static -static-libstdc++")
endif()
