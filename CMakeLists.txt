cmake_minimum_required( VERSION 3.28.0 )

project(
    A113
    VERSION 1.0.0
    LANGUAGES C CXX
)
set( A113_VERSION_MAJOR 1 )
set( A113_VERSION_MINOR 0 )
set( A113_VERSION_PATCH 3 )
set( A113_VERSION_STRING "a113v1.0.3" )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

set( A113_GENERATED_FILE_CAUTION_MSG "/*\n[A113] CAUTION!\nTHIS FILE WAS GENERATED DURING BUILD AND IT WILL BE OVERRIDEN IN THE NEXT ONE.\nDO NOT MODIFY AS THE MODIFICATIONS WILL BE LOST.\n*/")

option( A113_Opt_TargetPlate_OS OFF )
option( A113_Opt_TargetOS_Windows OFF )
option( A113_Opt_TargetOS_Linux OFF )

option( A113_Opt_TargetPlate_uC OFF )

option( A113_Opt_ConfirmClones OFF )


message( STATUS "[A113] CMake script begin..." )

if( NOT DEFINED A113_TARGET_PLATE )
    message( ERROR "[A113] Target plate not specified. Aborted." )
    return()
endif()

if( A113_TARGET_PLATE STREQUAL "OS" )
    set( A113_Opt_TargetPlate_OS ON )

    if( NOT DEFINED A113_TARGET_OS )
        message( ERROR "[A113] Target operating system not specified. Aborted." )
        return()
    endif()

    if( A113_TARGET_OS STREQUAL "Windows" )
        set( A113_Opt_TargetOS_Windows ON )
    elseif( A113_TARGET_OS STREQUAL "Linux" )
        set( A113_Opt_TargetOS_Linux ON )
    else()
        message( ERROR "[A113] Target operating system \"${A113_TARGET_OS}\" is not valid." )
        return()
    endif()

elseif( A113_TARGET_PLATE STREQUAL "uC" )
    set( A113_Opt_TargetPlate_uC ON )
else()
    message( ERROR "[A113] Target plate \"${A113_TARGET_PLATE}\" is not valid." )
    return()
endif()


include( FetchContent )
cmake_policy( SET CMP0169 OLD )

macro( A113_Mac__CloneRepo_Confirm )
    if( DEFINED A113_POPCOUNT_CLONES )
        string( SUBSTRING "${A113_POPCOUNT_CLONES}" 0 1 _popped )
        message( STATUS "[A113] Remaining clone popcount: ${A113_POPCOUNT_CLONES}." )
        string( SUBSTRING "${A113_POPCOUNT_CLONES}" 1 -1 _next_popcounts )
        set( A113_POPCOUNT_CLONES ${_next_popcounts} PARENT_SCOPE )
        if( NOT _popped STREQUAL "y" )
            message( STATUS "[A113] Bypassing repository \"${url}\", tag \"${tag}\"." )
            return()
        endif()
    endif()
endmacro()

function( A113_Fnc_JustCloneRepo url tag alias excom_dir )
    A113_Mac__CloneRepo_Confirm()

    message( STATUS "[A113] Cloning repository \"${url}\", tag \"${tag}\" to \"${excom_dir}/${alias}\"." )
    FetchContent_Declare(
        ${alias}
        GIT_REPOSITORY "${url}"
        SOURCE_DIR     "${excom_dir}/${alias}"
        GIT_TAG        "${tag}"
    )
    FetchContent_Populate( ${alias} )
endfunction( A113_Fnc_JustCloneRepo )

function( A113_Fnc_CloneRepo url tag alias excom_dir )
    A113_Mac__CloneRepo_Confirm()

    message( STATUS "[A113] Cloning repository \"${url}\", tag \"${tag}\" to \"${excom_dir}/${alias}\"." )
    FetchContent_Declare(
        ${alias}
        GIT_REPOSITORY "${url}"
        SOURCE_DIR     "${excom_dir}/${alias}"
        GIT_TAG        "${tag}"
    )
    FetchContent_MakeAvailable( ${alias} )
endfunction( A113_Fnc_CloneRepo )


message( STATUS "[A113] Adding the bedrock plate subdirectory." )
add_subdirectory( "brp" )
message( STATUS "[A113] Bedrock plate completed." )

if( A113_Opt_TargetPlate_uC )
    message( STATUS "[A113] Adding the micro-controller plate subdirectory." )
    add_subdirectory( "ucp" )
    message( STATUS "[A113] Micro-controller plate completed." )
endif()

if( A113_Opt_TargetPlate_OS )
    message( STATUS "[A113] Adding the operating-system plate subdirectory." )
    add_subdirectory( "osp" )
    message( STATUS "[A113] Operating-system plate completed." )
endif()


foreach( clockwork ${A113_MAKE_CLOCKWORKS} )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/clkwrk/${clockwork}" )
endforeach()

foreach( rupture ${A113_MAKE_RUPTURES} )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/ruptures/${rupture}" )
endforeach()

message( STATUS "[A113] CMake script done." )
