#pragma once

#include "generated.h"
#include "file.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define ___extern_res_start(file) extern const uint8_t PPCAT(file, _start)[] asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _start)))
#define ___extern_res_end(file)   extern const uint8_t PPCAT(file, _end)[] asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _end)))

#define __decl_memory_resource(file) \
___extern_res_start(file); \
___extern_res_end(file)

#define __checksum(name)  { \
	name \
}

#define __decl_res1(name, varname, endingsValue, humanName, contentTypeValue, cacheControl) \
http::resource::memory::file PPCAT(varname, _memory_file) = { \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	humanName, \
	contentTypeValue, \
    PPCAT(name, _checksum),  \
    cacheControl  \
};

#define __decl_res2(name, varname, endingsValue) \
http::resource::memory::file PPCAT(varname, _memory_file) = { \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	nullptr, \
	nullptr  \
};

#define __return_res1(name, endingsValue, humanName, contentTypeValue, cacheControl) \
http::resource::memory::file( \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	humanName, \
	contentTypeValue, \
	PPCAT(name, _checksum),  \
    cacheControl          	 \
);

#define __return_res2(name, endingsValue) \
http::resource::memory::file( \
	(int)PPCAT(name, _start), \
	(int)PPCAT(name, _end),\
	endingsValue, \
	nullptr, \
	nullptr \
);

#define decl_memory_file(fileV, endingV) \
__decl_memory_resource(fileV);           \
__decl_res2(fileV, fileV, endingV)

#if RESOURCE_COMPRESSION
#define _RESOURCE_COMPRESSED
#endif

#ifdef _RESOURCE_COMPRESSED

#define decl_web_resource(fileV, endingV, nameV, contentTypeV, cacheControl) \
__decl_memory_resource(PPCAT(fileV, _gz));                                	\
__decl_res1(PPCAT(fileV, _gz), fileV, http::resource::memory::endings::BINARY, nameV, contentTypeV, cacheControl)

#else

#define decl_web_resource(fileV, endingV, nameV, contentTypeV) \
__decl_memory_resource(fileV);                                \
__decl_res1(fileV, fileV, endingV, nameV, contentTypeV)

#endif

#define take_memory_file(fileV, endingV, nameV, contentTypeV) ({ http::resource::memory::file retval; __decl_memory_resource(fileV); retval = __return_res(fileV, endingV, nameV, contentTypeV); retval; })
