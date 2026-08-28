cmake_minimum_required(VERSION 3.24)

option(FULL_RELEASE "Set by your preset, DO NOT edit. Applies Release rules." OFF)
if(FULL_RELEASE AND CMAKE_CONFIGURATION_TYPES)
    message(STATUS "FULL_RELEASE enabled; ensure Release configuration is used")
endif()

# -------- Project ----------
project(
	RainExtinguishesFires
	VERSION 5.1.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
    Skyrim64Path
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition;installed path]"
    ABSOLUTE
)

set(Skyrim64Path "${Skyrim64Path}" CACHE PATH "Skyrim SE install directory")

if(NOT IS_DIRECTORY "${Skyrim64Path}")
    message(WARNING "Skyrim SE install path not found; Papyrus scripts will not be compiled or archived.")
endif()

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/Bethesda-CMakeModules/Modules"
)

set(VCPKG_OVERLAY_PORTS "")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/External/CommonLibSSE/cmake/ports/commonlib-shared/portfile.cmake")
	list(APPEND VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE/cmake/ports")
else()
	message(FATAL_ERROR "commonlib-shared overlay port not found in any CommonLibSSE checkout")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ports")
	list(APPEND VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ports")
endif()

include(SKSEPlugin)
set(SKSE_USE_XBYAK ON)
add_compile_definitions(SKSE_SUPPORT_XBYAK)
add_compile_definitions(SKYRIM_AE)
add_compile_definitions(SKYRIM_SUPPORT_AE)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")
find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
		"${PROJECT_NAME}"
		PRIVATE
			JsonCpp::JsonCpp
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CLIB_UTIL_INCLUDE_DIRS}
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/Data")
set(SKSE_DIR "${DATA_DIR}/SKSE")
set(SKSE_PLUGINS_DIR "${SKSE_DIR}/Plugins")

set(INI_PATH "${SKSE_PLUGINS_DIR}/${PROJECT_NAME}.ini")
set(PAPYRUS_SOURCE_DIR "${DATA_DIR}/Source/Scripts")
set(CUSTOM_CONSOLE_DIR "${SKSE_DIR}/CustomConsole")

if (IS_DIRECTORY PAPYRUS_SOURCE_DIR)
	install(
		DIRECTORY "${DATA_DIR}/Source"
		DESTINATION "."
		COMPONENT components
	)
endif()

if (IS_DIRECTORY CUSTOM_CONSOLE_DIR)
	install(
		DIRECTORY "${SKSE_DIR}/CustomConsole"
		DESTINATION "."
		COMPONENT components
	)
endif()

include(components.cmake)

# ---- Post build ----
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
	if (FULL_RELEASE)
		set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME} - Full Release")
	else()
		set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME} - Test")
	endif()
endif()

# ---- Compile Papyrus Scripts and Pack Into Archive ----
include(Papyrus)
include(BSArchive)

set(COPY_BSA OFF)
set(COPY_SCRIPTS  OFF)

if(EXISTS "${PAPYRUS_SOURCE_DIR}" AND IS_DIRECTORY "${Skyrim64Path}")
	file(GLOB ${PROJECT_NAME}_SOURCES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"${PAPYRUS_SOURCE_DIR}/*.psc"
	)

	Papyrus_Add(
		"Papyrus"
		GAME ${Skyrim64Path}
		IMPORTS ${PAPYRUS_SOURCE_DIR}
		SOURCES ${${PROJECT_NAME}_SOURCES}
		OPTIMIZE ANONYMIZE
	)

	BSArchive_Add(
		"BSA"
		OUTPUT "${PROJECT_NAME}.bsa"
		FORMAT SSE
		FILES ${Papyrus_OUTPUT}
	)
	
	set(COPY_SCRIPTS ON)
	if(FULL_RELEASE)
		set(COPY_BSA ON)
	endif()
	
elseif(EXISTS "${PAPYRUS_SOURCE_DIR}" AND NOT IS_DIRECTORY "${Skyrim64Path}")
	message(STATUS "Scripts exist in the Data directory, but Skyrim's path was not found, compilation will be skipped.")
else()
	message(STATUS "No scripts found to compile.")
endif()

# ---- Automatically copy artifacts to MO2 ----
if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

	set(POST_BUILD_COMMANDS
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"$<TARGET_FILE:${PROJECT_NAME}>"
			"${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"$<$<BOOL:$<TARGET_PDB_FILE:${PROJECT_NAME}>>:$<TARGET_PDB_FILE:${PROJECT_NAME}>>"
			"${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
		COMMAND "${CMAKE_COMMAND}"
			-DSRC_DIR=${DATA_DIR}
			-DDST_DIR=${OUTPUT_FOLDER}
			-DPROJECT_NAME=${PROJECT_NAME}
			-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Artifacts.cmake"
	)

	if(COPY_SCRIPTS)
		list(APPEND POST_BUILD_COMMANDS
			COMMAND "${CMAKE_COMMAND}"
				-DSOURCE_SCRIPTS=${PAPYRUS_SOURCE_DIR}
				-DOUTPUT_FOLDER=${OUTPUT_FOLDER}
				-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Sources.cmake"
		)
		list(APPEND POST_BUILD_COMMANDS
			COMMAND "${CMAKE_COMMAND}"
				-DCOMPILED_SCRIPTS="${CMAKE_BINARY_DIR}/Scripts"
				-DOUTPUT_FOLDER=${OUTPUT_FOLDER}
				-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Scripts.cmake"
		)

		if(COPY_BSA)
			list(APPEND POST_BUILD_COMMANDS
				COMMAND "${CMAKE_COMMAND}"
					-DSRC_DIR=${DATA_DIR}
					-DDST_DIR=${OUTPUT_FOLDER}
					-DPROJECT_NAME=${PROJECT_NAME}
					-DBINARY_DIR=${CMAKE_BINARY_DIR}
					-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Archive.cmake"
			)
		endif()
	endif()
	
	
	message(STATUS "==== Build Configuration ====")
	message(STATUS "FULL_RELEASE      = ${FULL_RELEASE}")
	message(STATUS "Skyrim64Path      = ${Skyrim64Path}")
	message(STATUS "OUTPUT_FOLDER     = ${OUTPUT_FOLDER}")
	message(STATUS "COPY_SCRIPTS      = ${COPY_SCRIPTS}")
	message(STATUS "COPY_BSA          = ${COPY_BSA}")
	message(STATUS "=============================")

	add_custom_command(
		TARGET "${PROJECT_NAME}"
		POST_BUILD
		${POST_BUILD_COMMANDS}
		VERBATIM
	)
else()
	message(STATUS "Output folder environment variable not set, build files will not be automatically deployed to MO2.")
endif()

install(
	FILES
		"${DATA_DIR}/RainExtinguishesFires.esp"
		"${DATA_DIR}/RainExtinguishesFires.bsa"
	DESTINATION "."
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/Source/Scripts/REF_FurnitureScript.psc"
		"${DATA_DIR}/Source/Scripts/REF_ObjectRefOffController.psc"
		"${DATA_DIR}/Source/Scripts/REF_QuestEventHandler.psc"
		"${DATA_DIR}/Source/Scripts/REF_RefAliasLoadManager.psc"
		"${DATA_DIR}/Source/Scripts/REF_UtilityFunctions.psc"
	DESTINATION "Source/Scripts"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/defaultEmbers.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/defaultFires.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/defaultGiantFires.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/defaultSmokes.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/embersXDCampfires.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/embersXDEmbers.json"
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires/embersXDGiantCampfires.json"
	DESTINATION "SKSE/Plugins/RainExtinguishesFires"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/RainExtinguishesFires.ini"
	DESTINATION "SKSE/Plugins"
	COMPONENT components
)