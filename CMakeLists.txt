# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(WebKeyboard)

option(RESOURCE_COMPRESSION "Enable resource compression" ON) #[[ changing default value does not affect cache so use full clean or delete ./build ]]
set (RESOURCE_SRC_DIR ${PROJECT_DIR}/resource)
set (RESOURCE_BIN_DIR ${CMAKE_BINARY_DIR}/resource)

message("RESOURCE_COMPRESSION=${RESOURCE_COMPRESSION}")

#[[configure_file(${COMPONENT_PATH}/make.h.template ${CMAKE_BINARY_DIR}/make.h @ONLY)]]
configure_file(${PROJECT_DIR}/main/make.h.template ${PROJECT_DIR}/main/make.h @ONLY)
#[[include_directories(${CMAKE_BINARY_DIR}/make.h)]]

if(RESOURCE_COMPRESSION)
    file(MAKE_DIRECTORY "${RESOURCE_BIN_DIR}")
    function(resource file type)
        message(STATUS "Processing resource: ${file}")
        file(ARCHIVE_CREATE PATHS "${RESOURCE_SRC_DIR}/${file}" OUTPUT "${RESOURCE_BIN_DIR}/${file}.gz" FORMAT raw COMPRESSION GZip COMPRESSION_LEVEL 9)
        target_add_binary_data(WebKeyboard.elf ${RESOURCE_BIN_DIR}/${file}.gz  BINARY)
    endfunction()
else()
    function(resource file type)
        message(STATUS "Processing resource: ${file}")
        target_add_binary_data(WebKeyboard.elf ${RESOURCE_SRC_DIR}/${file} ${type})
    endfunction()
endif()


include(resource/CMakeLists.txt)
