# Need vs 2019
cmake_minimum_required (VERSION 3.14.7)

# set the project name
project(ShangrenTools VERSION 1.0)

# Enbale code IDE to use folders, this will enable source_group(TREE,...)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#====================build_settings INTERFACE======================================#
# Add a interface library as build settings for all general targets
# All settings will be grouped into build-settings to avoid messing around with global settings
# Can be used by target_link_libraries(XXX PRIVATE build_settings) for a specific target
add_library(build_settings INTERFACE)
if (MSVC)
    target_compile_options(build_settings INTERFACE /W4 /WX /wd4100 /wd4189 /wd4324 /wd26812 /utf-8)
    # In all release modes also:
    # - disable w4189 - local variable is initialized but not referenced
    # - Enable whole program optimization (/GL)
    # - Enable string pooling (/GF)
    set(MSVC_ALL_RELEASE_COMPILE_OPTIONS /wd4189 /GL /GF)
    # In RELEASE mode:
    # - Set favor fast code option (/Ot)
    # - Enable intrinsic functions (/Oi)
	# - Enable full optimization (/Ox)
    set(MSVC_RELEASE_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Ot /Oi /Ox)
    # In MINSIZEREL mode set favor small code option (/Os)
    set(MSVC_MINSIZEREL_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Os)
    set(MSVC_RELWITHDEBINFO_COMPILE_OPTIONS ${MSVC_ALL_RELEASE_COMPILE_OPTIONS} /Ot /Oi /Ob2 /Ox)
    target_compile_options(build_settings INTERFACE "$<$<CONFIG:RELEASE>:${MSVC_RELEASE_COMPILE_OPTIONS}>")
    target_compile_options(build_settings INTERFACE "$<$<CONFIG:MINSIZEREL>:${MSVC_MINSIZEREL_COMPILE_OPTIONS}>")
    target_compile_options(build_settings INTERFACE "$<$<CONFIG:RELWITHDEBINFO>:${MSVC_RELWITHDEBINFO_COMPILE_OPTIONS}>")

    set(DEBUG_MACROS M_DEBUG)
    target_compile_definitions(build_settings INTERFACE "$<$<CONFIG:DEBUG>:${DEBUG_MACROS}>")
else()
    # TODO: The following need testing in MAC OS 
endif()

# UNICODE support
target_compile_definitions(build_settings INTERFACE UNICODE _UNICODE)
#====================End build_settings INTERFACE==================================#

#====================Helper functions==============================================#
if(WIN32)
    # Helper function for configuring Windows App
    function(add_target_platform_app TARGET_NAME WINDOWED SOURCE ASSETS RESFILES)
        set_source_files_properties(${ASSETS} PROPERTIES 
            VS_DEPLOYMENT_LOCATION  "."
        )
        if(${WINDOWED})
            add_executable(${TARGET_NAME} WIN32 "${SOURCE}" "${RESFILES}" "${ASSETS}")
        else()
            add_executable(${TARGET_NAME} "${SOURCE}" "${RESFILES}" "${ASSETS}")
        endif()

        set_target_properties(${TARGET_NAME} PROPERTIES 
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endfunction()
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Helper function for configuring target properties
function(config_target_properties TARGET)
    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if(MSVC)
        # For msvc, enable link-time code generation for release builds. https://docs.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=vs-2019
        if(TARGET_TYPE STREQUAL STATIC_LIBRARY)
            foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
                set_target_properties(${TARGET} PROPERTIES
                    STATIC_LIBRARY_FLAGS_${REL_CONFIG} /LTCG
                )
            endforeach()
        else()
        # https://docs.microsoft.com/en-us/cpp/build/reference/incremental-link-incrementally?view=vs-2019
            foreach(REL_CONFIG ${RELEASE_CONFIGURATIONS})
                set_target_properties(${TARGET} PROPERTIES
                    LINK_FLAGS_${REL_CONFIG} "/LTCG /OPT:REF /INCREMENTAL:NO"
                )
            endforeach()
        endif()
    endif()
endfunction()

# Helper function for adding app target
function(add_app APP_NAME IDE_FOLDER WINDOWED SOURCE ASSETS RESFILES)
    add_target_platform_app(${APP_NAME} "${WINDOWED}" "${SOURCE}" "${ASSETS}" "${RESFILES}")
    config_target_properties(${APP_NAME})
    # Config the folder in Code IDE
    set_target_properties(${APP_NAME} PROPERTIES
        FOLDER ${IDE_FOLDER}
    )
endfunction()
#====================End Helper functions===========================================#

add_subdirectory(TianyuanCalcCMD)

# Config visual studio default startup project
if(TARGET TianyuanCalcCMD)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT TianyuanCalcCMD)
endif()
