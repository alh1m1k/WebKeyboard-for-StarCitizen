# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(WebKeyboard)

option(RESOURCE_COMPRESSION "Enable resource compression" ON) #[[ changing default value does not affect cache so use full clean or delete ./build ]]
option(RESOURCE_CHECKSUM    "Checksum resources" ON) #[[ changing default value does not affect cache so use full clean or delete ./build ]]
option(EMBED_CERT           "Include ssl certificate" OFF) #[[ changing default value does not affect cache so use full clean or delete ./build ]]

set (WEB_ROOT_SRC ${PROJECT_DIR}/resource)
set (WEB_ROOT_BIN ${CMAKE_BINARY_DIR}/resource)
set (DISCLAIMER "do not touch! content is auto generated on every build!")
set (EMBED_CACERT OFF)

file(REMOVE "${PROJECT_DIR}/main/resourceChecksum.h")
file(APPEND "${PROJECT_DIR}/main/resourceChecksum.h" "#pragma once \n\n \/\/${DISCLAIMER}\n\n")
if(RESOURCE_COMPRESSION)
    file(MAKE_DIRECTORY "${WEB_ROOT_BIN}")
endif ()

if (EMBED_CERT)
    target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/cert.pem"        TEXT)
    target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/privkey.pem"     TEXT)
    if(EXISTS "${PROJECT_DIR}/cert/cacert.pem")
        target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/cacert.pem"  TEXT)
        set(EMBED_CACERT ON)
    endif ()
endif ()

message("RESOURCE_COMPRESSION=${RESOURCE_COMPRESSION}")
message("RESOURCE_CHECKSUM=${RESOURCE_CHECKSUM}")
message("EMBED_CERT=${EMBED_CERT}")
message("EMBED_CACERT=${EMBED_CACERT}")

#[[configure_file(${COMPONENT_PATH}/make.h.template ${CMAKE_BINARY_DIR}/make.h @ONLY)]]
configure_file(${PROJECT_DIR}/main/make.h.template ${PROJECT_DIR}/main/make.h @ONLY)
#[[include_directories(${CMAKE_BINARY_DIR}/make.h)]]

function(appendHashValue file value)
    get_filename_component(name "${file}" NAME)
    string(MAKE_C_IDENTIFIER "${name}_checksum" varname)
    file(APPEND "${PROJECT_DIR}/main/resourceChecksum.h" "#define ${varname} ${value}\n")
endfunction()

function(resourceHash file out)
    file(SHA256 ${file} hashValue)
    appendHashValue(${file} \"${hashValue}\")
    set(${out} ${hashValue} PARENT_SCOPE)
endfunction()

#there is problem when hierarchical fs structure collapse in to single directory
#currently this is work, but later is must be changed to random prefixes or hierarchical subdirectories or optional new name parameter
#it will continue to work until file name unique
function(resource file type)
    if (RESOURCE_COMPRESSION)
        get_filename_component(fileName ${file} NAME)
        set(embedFile "${WEB_ROOT_BIN}/${fileName}.gz")
        set(embedType BINARY)
        get_filename_component(path "${embedFile}" PATH)
        file(MAKE_DIRECTORY ${path})
        file(ARCHIVE_CREATE PATHS ${file} OUTPUT ${embedFile} FORMAT raw COMPRESSION GZip COMPRESSION_LEVEL 9)
    else ()
        set(embedFile  ${file})
        set(embedType  ${type})
    endif ()
    target_add_binary_data(WebKeyboard.elf "${embedFile}" ${embedType})
    get_filename_component(embedName "${embedFile}" NAME)
    if (RESOURCE_CHECKSUM)
        resourceHash(${embedFile} hashVal)
        message(STATUS "Processing resource: ${embedName} hash: ${hashVal}")
    else ()
        appendHashValue(${embedFile} nullptr)
        message(STATUS "Processing resource: ${embedName}")
    endif ()
endfunction()

function(temporal directory)
    if(NOT IS_DIRECTORY ${directory})
        file(MAKE_DIRECTORY "${directory}")
        add_custom_command(
            TARGET WebKeyboard.elf
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -r "${directory}"
            COMMENT "remove ${directory}"
        )
    endif ()
endfunction()

include(resource/CMakeLists.txt)
