cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
project(avbot)

#-----------------------------------------------------------------------------
# force off-tree build
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "do not run cmake inside avbot source directory
Remove the CMakeCache.txt file and try again from another folder, e.g.:
   rm CMakeCache.txt
   mkdir make
   cd make
   cmake ..
")
endif()


set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(cpacksettings.cmake)
INCLUDE(CPack)

include(cxx11.cmake)
include(tweakvcflasgs.cmake)

#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
option(ENABLE_STATIC "link to boost staticaly" ON)
option(ENABLE_TEST "build xmpp/pop3 test" OFF)
option(ENABLE_LIBSTDCXX_STATIC "link to static libstdc++" OFF)
option(ENABLE_LIBGCC_STATIC "link to static libgcc" OFF)
option(WITH_ICONV "boost is build with iconv" ON)
option(WITH_ICU "boost is build with icu " OFF)
option(WITH_QT_GUI "a qt gui interface" OFF)

if(WIN32)
	set(ENABLE_LIBGCC_STATIC ON)
	set(ENABLE_LIBSTDCXX_STATIC ON)
	set(ENABLE_STATIC ON)
if(MSVC)
	set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} DbgHelp.lib secur32.lib ws2_32.lib comctl32.lib mswsock.lib winmm.lib crypt32.lib")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:winmm.dll")
else()
	link_libraries(dbghelp secur32 ws2_32  comctl32  mswsock winmm crypt32)
endif()
endif()

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501 -DWINVER=0x0501)
	if(NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -g0")
	endif()

	if(MSVC)
		if(CMAKE_CL_64)
			link_directories(${PROJECT_SOURCE_DIR}/openssl/lib64)
		else()
			link_directories(${PROJECT_SOURCE_DIR}/openssl/lib)
		endif()

	else()
		link_directories(${PROJECT_SOURCE_DIR}/openssl/lib64)
		link_directories(${PROJECT_SOURCE_DIR}/openssl/lib)
	endif()
	link_directories(${PROJECT_SOURCE_DIR}/libsformingw64)
endif()

if(NOT MSVC AND NOT APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g0 -O3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g0 -O3")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
endif()

if(CMAKE_BUILD_TYPE EQUAL "TEST")
	set(ENABLE_STATIC ON)
endif()

if(ENABLE_STATIC)
	set(Boost_USE_STATIC_LIBS        ON)
	set(Boost_USE_STATIC_RUNTIME    ON)
else(ENABLE_STATIC)
	ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
endif()

if(ENABLE_LIBSTDCXX_STATIC OR WIN32)
	if(NOT MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ ")
	endif()

	set(Boost_USE_STATIC_LIBS        ON)
	set(Boost_USE_STATIC_RUNTIME    ON)
endif()

if(ENABLE_LIBGCC_STATIC OR WIN32)
	if(NOT MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc ")
	endif()
endif()

find_package(Boost 1.55 REQUIRED COMPONENTS timer chrono date_time filesystem system program_options regex locale)

if(MSVC)
	# add_definitions( -DBOOST_ALL_NO_LIB )
	link_directories(${Boost_LIBRARY_DIR})
endif()

#add_definitions(-DBOOST_LOG_NO_THREADS)
find_package(Threads)

if(WIN32)
	add_definitions(-DBOOST_ALL_STATIC_LINK)
	add_definitions(-DBOOST_THREAD_USE_LIB)
	add_definitions(-DBOOST_FILESYSTEM_STATIC_LINK)
	add_definitions(-DBOOST_ALL_STATIC_LINK)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	add_definitions(-DNOMINMAX)
endif()

find_package(systemd)

add_definitions(-DAVHTTP_ENABLE_OPENSSL -DAVHTTP_DISABLE_MULTI_DOWNLOAD -DDISABLE_LOGGER)

configure_file(avbot.rc.in ${PROJECT_BINARY_DIR}/avbot.rc)
configure_file(avbot.service.in ${PROJECT_BINARY_DIR}/avbot.service)

add_subdirectory(sqlite)

if(CMAKE_MAJOR_VERSION LESS 3)
get_target_property(sqlite_lib_location sqlite3 LOCATION)
else()
set(sqlite_lib_location  $<TARGET_LINKER_FILE:sqlite3>)
endif()
message (STATUS "sqlite_lib_location == ${sqlite_lib_location}")

set(SQLITE_ROOT_DIR "${PROJECT_BINARY_DIR}/sqlite/")
set(SQLITE3_INCLUDES "${PROJECT_SOURCE_DIR}/sqlite/include/" CACHE PATH "include dir for sqlite3.h")
set(SQLITE3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/sqlite/include/")
set(SQLITE3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/sqlite/include/" CACHE PATH "include dir for sqlite3.h" FORCE)
set(SQLITE3_LIBRARIES $<TARGET_LINKER_FILE:sqlite3>)
set(SQLITE3_LIBRARIES $<TARGET_LINKER_FILE:sqlite3> CACHE FILEPATH "lib file to link to sqlite")
set(SQLITE3_LIBRARY "")
set(SQLite3_FOUND ON)

add_subdirectory(soci/src EXCLUDE_FROM_ALL)

if(NOT WIN32)
	find_package(OpenSSL)
else()
	set(OpenSSL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/openssl/include)
	include_directories("${PROJECT_SOURCE_DIR}/openssl/include")
	include_directories("${PROJECT_SOURCE_DIR}/openssl/include")
endif(NOT WIN32)

if(WITH_QT_GUI)
	add_subdirectory(gui/qt-gui)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(soci/src/core)
include_directories(soci/src/backends/sqlite3)
include_directories(sqlite/include)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(avboost/include)
include_directories(avhttp/include)
include_directories(libwebqq/include)
include_directories(deCAPTCHA/include)
include_directories(avhttpd/include)

add_subdirectory(libwebqq)
add_subdirectory(libirc)
add_subdirectory(libxmpp)
add_subdirectory(libmailexchange)
add_subdirectory(deCAPTCHA)
add_subdirectory(libavbot)
add_subdirectory(libavlog)
add_subdirectory(extension)

configure_file(version.c.in version.c @ONLY)

set(AVBOT_SOURCES
	auto_welcome.hpp
	avbot_vc_feed_input.hpp
	botctl.hpp
	counter.hpp
	input.hpp
	main.cpp
	botctl.cpp
	input.cpp
	avbot_log_search.cpp
	protocoladapters.cpp
	${PROJECT_BINARY_DIR}/version.c
	${PROJECT_BINARY_DIR}/avbot.rc
)

set(AVBOTRPC_SOURCES
	rpc/server.cpp
	rpc/server.hpp
	)

if(NOT WIN32)
	list(APPEND AVBOT_SOURCES backtrace.cpp)
endif()

if(WIN32)
	list(APPEND AVBOT_SOURCES minidump.cpp)
endif()

if(WIN32)
	list(APPEND AVBOT_SOURCES sound.cpp )
else()

	find_package(ALSA)

	if (ALSA_FOUND)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tweet.o
			COMMAND ${CMAKE_LINKER} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/tweet.o tweet.wav
			DEPENDS tweet.wav WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			)

		include_directories(${ALSA_INCLUDE_DIR})
		list(APPEND AVBOT_SOURCES sound_alsa.cpp)
		list(APPEND AVBOT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/tweet.o )
	else()
		list(APPEND AVBOT_SOURCES sound_dumy.cpp)
	endif()

endif()

if(MSVC)

	if(CMAKE_CL_64)
		LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x64/")
	else()
		LINK_DIRECTORIES( "${CMAKE_CURRENT_SOURCE_DIR}/extension/luascript/msvc_x86/")
	endif()

endif()

add_executable(avbot ${AVBOT_SOURCES} ${AVBOTRPC_SOURCES})

if(NOT WIN32)
	target_link_libraries(avbot -rdynamic)
endif()

if(WIN32)
	add_subdirectory(gui/win32-gui)

	if(MSVC)
		target_link_libraries(avbot ssleay32.lib libeay32.lib zlib.lib)
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lcrypt32 -lmswsock")
		target_link_libraries(avbot -lssl -lcrypto)
	endif()
	add_definitions(-U__STRICT_ANSI__)

	if(WITH_ICONV AND NOT MSVC)
		target_link_libraries(avbot -liconv)
	endif()

else()
	target_link_libraries(avbot ${OPENSSL_LIBRARIES} )
endif()

if(APPLE AND WITH_ICONV)
	target_link_libraries(avbot -liconv)
endif()

target_link_libraries(avbot libavbot avlog extension soci_core_static soci_sqlite3_static sqlite3)

target_link_libraries(avbot ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if( NOT Boost_USE_STATIC_RUNTIME )
	message(STATUS "dynamic link to boost is not recomanded")
endif()

check_library_exists(rt clock_gettime "" HAVE_CLOCK_GETTIME)
if(HAVE_CLOCK_GETTIME)
	target_link_libraries(avbot -lrt)
endif(HAVE_CLOCK_GETTIME)

if (NOT WIN32)
	set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/soci/src/cmake/modules ${CMAKE_MODULE_PATH})
	find_package(DL)
	if(DL_FOUND)
		include_directories(${DL_INCLUDE_DIR})
		target_link_libraries(avbot ${DL_LIBRARY})
	endif()
endif()

if (WITH_QT_GUI OR WIN32)
	if(WITH_QT_GUI)
		set_target_properties(avbot PROPERTIES COMPILE_FLAGS "-DWITH_QT_GUI")
	endif()
	target_link_libraries(avbot avbotgui)
endif()

if (ALSA_FOUND)
	target_link_libraries(avbot ${ALSA_LIBRARY})
endif()

if(SYSTEMD_FOUND)
	target_link_libraries(avbot ${SYSTEMD_LIBRARY})
	add_definitions(-DHAVE_SYSTEMD)
endif()

install(TARGETS avbot RUNTIME DESTINATION bin)
if(NOT WIN32)
	install(FILES ${PROJECT_BINARY_DIR}/avbot.service DESTINATION /lib/systemd/system)
endif()
