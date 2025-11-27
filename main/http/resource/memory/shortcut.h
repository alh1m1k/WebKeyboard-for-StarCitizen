#pragma once

#include "generated.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define ___extern_res_start(file) extern const uint8_t PPCAT(file, _start)[] asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _start)))
#define ___extern_res_end(file)   extern const uint8_t PPCAT(file, _end)[] asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _end)))

#define __decl_memory_resource(file) \
___extern_res_start(file); \
___extern_res_end(file)

#define __decl_res(name, varname, endingsValue, humanName, contentTypeValue) \
http::resource::memory::file PPCAT(varname, _memory_file) = { \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	humanName, \
	contentTypeValue, \
    PPCAT(name, _checksum) \
};

#define __return_res(name, endingsValue, humanName, contentTypeValue) \
http::resource::memory::file( \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	humanName, \
	contentTypeValue, \
	PPCAT(name, _checksum)  \
);

#if RESOURCE_COMPRESSION
#define _RESOURCE_COMPRESSED
#endif

#ifdef _RESOURCE_COMPRESSED

#include "file.h"
#define decl_memory_file(fileV, endingV, nameV, contentTypeV) \
__decl_memory_resource(PPCAT(fileV, _gz));                                \
__decl_res(PPCAT(fileV, _gz), fileV, http::resource::memory::endings::BINARY, nameV, contentTypeV)

#else

#define decl_memory_file(fileV, endingV, nameV, contentTypeV) \
__decl_memory_resource(fileV);                                \
__decl_res(fileV, fileV, endingV, nameV, contentTypeV)

#endif

#define take_memory_file(fileV, endingV, nameV, contentTypeV) ({ http::resource::memory::file retval; __decl_memory_resource(fileV); retval = __return_res(fileV, endingV, nameV, contentTypeV); retval; })
