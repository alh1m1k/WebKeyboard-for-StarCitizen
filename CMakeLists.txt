# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(WebKeyboard)

option(RESOURCE_COMPRESSION "Enable resource compression" ON) #[[ changing default value does not affect cache so use full clean or delete ./build ]]
option(RESOURCE_CHECKSUM    "Checksum resources" ON) #[[ changing default value does not affect cache so use full clean or delete ./build ]]
option(EMBED_CERT           "Include ssl certificate" OFF) #[[ changing default value does not affect cache so use full clean or delete ./build ]]

set (RESOURCE_SRC_DIR ${PROJECT_DIR}/resource)
set (RESOURCE_BIN_DIR ${CMAKE_BINARY_DIR}/resource)
set (DISCLAIMER "do not touch! content is auto generated on every build!")
set (EMBED_CACERT OFF)

message("RESOURCE_COMPRESSION=${RESOURCE_COMPRESSION}")
message("RESOURCE_CHECKSUM=${RESOURCE_CHECKSUM}")

#[[configure_file(${COMPONENT_PATH}/make.h.template ${CMAKE_BINARY_DIR}/make.h @ONLY)]]
configure_file(${PROJECT_DIR}/main/make.h.template ${PROJECT_DIR}/main/make.h @ONLY)
#[[include_directories(${CMAKE_BINARY_DIR}/make.h)]]

file(REMOVE "${PROJECT_DIR}/main/resourceChecksum.h")
file(APPEND "${PROJECT_DIR}/main/resourceChecksum.h" "#pragma once \n\n \/\/${DISCLAIMER}\n\n")
if(RESOURCE_COMPRESSION)
    file(MAKE_DIRECTORY "${RESOURCE_BIN_DIR}")
endif ()

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

function(resource file type)
    if (RESOURCE_COMPRESSION)
        set(embedFile "${RESOURCE_BIN_DIR}/${file}.gz")
        set(embedType BINARY)
        get_filename_component(path "${embedFile}" PATH)
        file(MAKE_DIRECTORY ${path})
        file(ARCHIVE_CREATE PATHS "${RESOURCE_SRC_DIR}/${file}" OUTPUT ${embedFile} FORMAT raw COMPRESSION GZip COMPRESSION_LEVEL 9)
    else ()
        set(embedFile "${RESOURCE_SRC_DIR}/${file}")
        set(embedType ${type})
    endif ()
    target_add_binary_data(WebKeyboard.elf "${embedFile}" ${embedType})
    if (RESOURCE_CHECKSUM)
        resourceHash(${embedFile} hashVal)
        get_filename_component(embedName "${embedFile}" NAME)
        message(STATUS "Processing resource: ${file}(${embedName}) hash: ${hashVal}")
    else ()
        appendHashValue(${embedFile} nullptr)
        message(STATUS "Processing resource: ${file}")
    endif ()
endfunction()

if (EMBED_CERT)
    target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/cert.pem"        TEXT)
    target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/privkey.pem"     TEXT)
    if(EXISTS "${PROJECT_DIR}/cert/cacert.pem")
        target_add_binary_data(WebKeyboard.elf "${PROJECT_DIR}/cert/cacert.pem"  TEXT)
        set(EMBED_CACERT ON)
    endif ()
endif ()

message("EMBED_CERT=${EMBED_CERT}")
message("EMBED_CACERT=${EMBED_CACERT}")


include(resource/CMakeLists.txt)
