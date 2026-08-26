#pragma once

#include "generated.h"
#include "file.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define IIF_CAT(a, ...) PRIMITIVE_CAT(a, __VA_ARGS__)
#define IIF_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
#define IIF(c) IIF_PRIMITIVE_CAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t


#define ___extern_res_start(file) extern const uint8_t PPCAT(file, _start)[] 	asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _start)))
#define ___extern_res_end(file)   extern const uint8_t PPCAT(file, _end)[] 		asm(STRINGIZE(PPCAT(PPCAT(_binary_, file), _end)))

#define __decl_memory_resource(file)	\
___extern_res_start(file);				\
___extern_res_end(file)


#define __decl_res1(name, varname, bitmask, humanName, contentTypeValue, cacheControl)  \
http::resource::file PPCAT(varname, _memory_file) = {									\
	(int)PPCAT(name, _start),															\
	(int)PPCAT(name, _end),																\
	bitmask,																			\
	humanName,																			\
	contentTypeValue,																	\
    PPCAT(name, _checksum),																\
    cacheControl																		\
};

#define __decl_res2(name, varname, bitmask)										\
http::resource::file PPCAT(varname, _memory_file) = {							\
	(int)PPCAT(name, _start),													\
	(int)PPCAT(name, _end),														\
	bitmask,																	\
	nullptr,																	\
	nullptr,																	\
	nullptr																		\
};

#define __return_res1(name, bitmask, humanName, contentTypeValue, cacheControl) \
http::resource::file(															\
	(int)PPCAT(name, _start),													\
	(int)PPCAT(name, _end),														\
	bitmask,																	\
	humanName,																	\
	contentTypeValue,															\
	PPCAT(name, _checksum),														\
    cacheControl          														\
);

#define __return_res2(name, bitmask)	\
http::resource::file(					\
	(int)PPCAT(name, _start),			\
	(int)PPCAT(name, _end),				\
	bitmask,							\
	nullptr,							\
	nullptr,							\
    nullptr								\
);

#define decl_memory_file(fileV, bitmask) \
__decl_memory_resource(fileV);           \
__decl_res2(fileV, fileV, bitmask)

#define decl_web_resource_compressed(fileV, bitmap, nameV, contentTypeV, cacheControl)															\
__decl_memory_resource(PPCAT(fileV, _gz));                                																		\
__decl_res1(PPCAT(fileV, _gz), fileV, (bitmap)|http::resource::TYPE_BINARY|http::resource::ATTR_COMPRESSED, nameV, contentTypeV, cacheControl)

#define decl_web_resource_uncompressed(fileV, bitmap, nameV, contentTypeV, cacheControl)	\
__decl_memory_resource(fileV);																\
__decl_res1(fileV, fileV, bitmap, nameV, contentTypeV, cacheControl)

#define decl_web_resource_compressed_if(compressedIf, fileV, bitmap, nameV, contentTypeV, cacheControl)	\
IIF(compressedIf)(																				\
	decl_web_resource_compressed(fileV, bitmap, nameV, contentTypeV, cacheControl),				\
	decl_web_resource_uncompressed(fileV, bitmap, nameV, contentTypeV, cacheControl)			\
)

#define decl_web_resource(fileV, bitmap, nameV, contentTypeV, cacheControl) decl_web_resource_compressed_if(RESOURCE_COMPRESSION, fileV, bitmap, nameV, contentTypeV, cacheControl)

#define take_memory_file(fileV, bitmap, nameV, contentTypeV) ({ http::resource::file retval; __decl_memory_resource(fileV); retval = __return_res(fileV, bitmap, nameV, contentTypeV); retval; })
