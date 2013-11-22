cmake_minimum_required(VERSION 2.6)

project(KetcherServer)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/indigo/common/cmake/)
INCLUDE(ConfigureCommon)

ENABLE_TESTING()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/indigo/common/cmake/ ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)
file(GLOB KetcherServer_src src/*.c*)
file(GLOB KetcherServer_headers src/*.h)

include_directories(src)
add_subdirectory(third_party/indigo/build_scripts/indigo-all "${CMAKE_CURRENT_BINARY_DIR}/indigo-all")
set(Indigo_LIBRARIES indigo indigo-inchi indigo-renderer inchi png z pixman cairo tinyxml molecule layout graph reaction common render2d)
set(Indigo_LIBRARIES ${Indigo_LIBRARIES} ${Indigo_LIBRARIES} ${Indigo_LIBRARIES})


include_directories(third_party/indigo/common)
include_directories(third_party/indigo/api)
include_directories(third_party/indigo/api/plugins/renderer)
include_directories(third_party/indigo/api/plugins/inchi)

add_library(ketcher-server-shared SHARED ${KetcherServer_src} ${KetcherServer_headers})
if (UNIX AND NOT APPLE)
	if(${SUBSYSTEM_NAME} MATCHES "x64")
		SET(LINK_FLAGS "${LINK_FLAGS} -Wl,--wrap=memcpy " )
	endif()
endif()
target_link_libraries(ketcher-server-shared ${Indigo_LIBRARIES})
SET_TARGET_PROPERTIES(ketcher-server-shared PROPERTIES OUTPUT_NAME "ketcher-server")

if(APPLE)
	SET(LINK_FLAGS "${LINK_FLAGS} -Xlinker -framework -Xlinker ApplicationServices -Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/ketcher_server.explist ")
endif()

if(UNIX AND NOT APPLE)
	SET(LINK_FLAGS "${LINK_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ketcher_server.so.map ")
endif()

set_target_properties(ketcher-server-shared PROPERTIES LINK_FLAGS "${LINK_FLAGS}")

PACK_SHARED(ketcher-server-shared)

SET(CMAKE_INSTALL_PREFIX "install")
SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
INCLUDE(InstallRequiredSystemLibraries)
FOREACH(lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
	INSTALL(FILES ${lib} DESTINATION "shared/${SYSTEM_NAME}/${SUBSYSTEM_NAME}" COMPONENT shared)
ENDFOREACH()
