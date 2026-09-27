cmake_minimum_required(VERSION 3.24)

option(COPY_BSA "Copies any generated BSA to the MO2 directory (if specified)." OFF)
option(COPY_PEX "Copies any generated scripts to the MO2 directory (if specified). Applies after the BSA (if enabled)." ON)
option(COPY_PSC "Copies any script sources to the MO2 directory (if specified). Applies after the BSA (if enabled)." ON)

# -------- Project ----------
project(
	RainExtinguishesFires
	VERSION 5.1.2
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
	Skyrim64Path
	"[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition;installed path]"
	ABSOLUTE CACHE
)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/Bethesda-CMakeModules/Modules"
)

include(SKSEPlugin)
set(SKSE_USE_XBYAK ON)
find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")

add_compile_definitions(SKSE_SUPPORT_XBYAK)
add_compile_definitions(SKYRIM_AE)
add_compile_definitions(SKYRIM_SUPPORT_AE)

set(SKSE_COMMONLIBSSE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/External/CommonLibSSE")
set(SKSE_NO_INSTALL ON)

if (GOG)
	add_compile_definitions(SKYRIM_GOG)
	set(SKSE_COMMONLIBSSE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/External/CommonLibSSE_1.6")
endif()

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	INCLUDE_DIR include
	SOURCES
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
		"${PROJECT_NAME}"
		PRIVATE
			JsonCpp::JsonCpp
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CLIB_UTIL_INCLUDE_DIRS}
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

include(components.cmake)

# ---- Post build ----
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
	if(GOG)
		set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME} - GOG")
	elseif (FULL_RELEASE)
		set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME} - Full Release")
	else()
		set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME} - Test")
	endif()
endif()

# ---- Compile Papyrus Scripts and Pack Into Archive ----
include(Papyrus)
include(BSArchive)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Data/Source/Scripts" AND NOT SKIP_ARTIFACTS)
	file(GLOB ${PROJECT_NAME}_SOURCES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"Data/Source/Scripts/*.psc"
	)

	Papyrus_Add(
		"Papyrus"
		GAME ${Skyrim64Path}
		IMPORTS ${CMAKE_CURRENT_SOURCE_DIR}/Data/Source/Scripts
		SOURCES ${${PROJECT_NAME}_SOURCES}
		OPTIMIZE ANONYMIZE
	)

	BSArchive_Add(
		"BSA"
		OUTPUT "${PROJECT_NAME}.bsa"
		FORMAT SSE
		FILES ${Papyrus_OUTPUT}
	)
else()
	message(STATUS "No scripts to compile found, won't compress into BSA.")
endif()

# ---- Automatically copy artifacts to MO2 ----
if(DEFINED OUTPUT_FOLDER AND NOT SKIP_ARTIFACTS)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"

		COMMAND "${CMAKE_COMMAND}"
			-DSRC_DIR="${PROJECT_SOURCE_DIR}/Data"
			-DDST_DIR="${OUTPUT_FOLDER}"
			-DPROJECT_NAME="${PROJECT_NAME}"
			-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Artifacts.cmake"
    )

    if(COPY_BSA)
		add_custom_command(
			TARGET "${PROJECT_NAME}"
			POST_BUILD
			COMMAND "${CMAKE_COMMAND}"
				-DSRC_DIR="${PROJECT_SOURCE_DIR}/Data"
				-DDST_DIR=${OUTPUT_FOLDER}
				-DPROJECT_NAME=${PROJECT_NAME}
				-DBINARY_DIR=${CMAKE_BINARY_DIR}
				-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Archive.cmake"
			VERBATIM
		)
	endif()

    if(COPY_PEX)
        add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            -DCOMPILED_SCRIPTS="${CMAKE_BINARY_DIR}/Scripts"
            -DOUTPUT_FOLDER="${OUTPUT_FOLDER}"
            -P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Scripts.cmake"
        VERBATIM
		)
    endif()

	if(COPY_PSC)
        add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            -DSOURCE_SCRIPTS="${PROJECT_SOURCE_DIR}/Data/Source/Scripts"
            -DOUTPUT_FOLDER="${OUTPUT_FOLDER}"
            -P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Sources.cmake"
        VERBATIM
		)
    endif()
endif()

# ---- Github Release Stuff ----
install(
	FILES
		"$<TARGET_FILE:${PROJECT_NAME}>"
		"$<TARGET_PDB_FILE:${PROJECT_NAME}>"
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}.ini"
	DESTINATION "01. Core/SKSE/Plugins"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/${PROJECT_NAME}.esp"
		"${DATA_DIR}/${PROJECT_NAME}.bsa"
	DESTINATION "01. Core"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/defaultFires.json"
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/embersXDCampfires.json"
	DESTINATION "02. Fire Options/01. Campfires/SKSE/Plugins/${PROJECT_NAME}/"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/defaultEmbers.json"
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/embersXDEmbers.json"
	DESTINATION "02. Fire Options/02. Ember Fires/SKSE/Plugins/${PROJECT_NAME}/"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/defaultGiantFires.json"
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/embersXDGiantCampfires.json"
	DESTINATION "02. Fire Options/03. Giant Fires/SKSE/Plugins/${PROJECT_NAME}/"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/SKSE/Plugins/${PROJECT_NAME}/defaultSmokes.json"
	DESTINATION "02. Fire Options/04. Smoke Objects/SKSE/Plugins/${PROJECT_NAME}/"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/Source/Scripts/REF_FurnitureScript.psc"
		"${DATA_DIR}/Source/Scripts/REF_ObjectRefOffController.psc"
		"${DATA_DIR}/Source/Scripts/REF_QuestEventHandler.psc"
		"${DATA_DIR}/Source/Scripts/REF_RefAliasLoadManager.psc"
		"${DATA_DIR}/Source/Scripts/REF_UtilityFunctions.psc"
	DESTINATION "03. Sources/Source/Scripts"
	COMPONENT components
)

install(
	FILES
		"${DATA_DIR}/RainExtinguishesEmbersXD.esp"
	DESTINATION "04. Patches/Embers XD"
	COMPONENT components
)

install(
	FILES
		"${PROJECT_SOURCE_DIR}/FOMOD/info.xml"
		"${PROJECT_SOURCE_DIR}/FOMOD/ModuleConfig.xml"
	DESTINATION "FOMOD"
	COMPONENT components
)

install(
	FILES
		"${PROJECT_SOURCE_DIR}/FOMOD/Images/REF_Header.jpg"
	DESTINATION "FOMOD/Images"
	COMPONENT components
)