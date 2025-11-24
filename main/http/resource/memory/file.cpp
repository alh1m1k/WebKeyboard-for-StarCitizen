#include "file.h"
#include "generated.h"


#include "contentType.h"
#include "resource.h"
#include "util.h"
#include <sys/_stdint.h>

namespace http::resource::memory {

	//file::file(int addressStart, int addressEnd, endings end, const char * name) : addressStart(addressStart), addressEnd(addressEnd), ending((int)end), name(name) {};
	
	file::file(int addressStart, int addressEnd, endings end, const char * name, const char * contentType) 
		: addressStart(addressStart), addressEnd(addressEnd), ending((int)end), name(name), contentType(contentType) {};
		
	file::file(int addressStart, int addressEnd, endings end, const char * name, enum contentType ct) 
		: addressStart(addressStart), addressEnd(addressEnd), ending((int)end), name(name), contentType(contentType2Symbols(ct)) {};
	
	bool file::operator==(const file& other) const {
		return other.name == this->name;
	}
	
	bool file::operator!=(const file& other) const {
		return other.name != this->name;
	}
	
	bool file::operator<(const file& other) const {
		return other.name < this->name;
	}
	
	handlerRes file::operator()(request& req, response& resp, server& serv) {

#ifdef RESOURCE_COMPRESSED
		if (req.getHeaders().acceptEncoding().contains("gzip")) {
			resp.getHeaders().contentEncoding("gzip");
		} else {
			throw not_impleneted("runtime decompression");
		}
#endif

        ssize_t size = addressEnd - addressStart;
		resp.getHeaders().contentType(this->contentType);
        if (size > RESPONSE_MAX_UNCHUNKED_SIZE) {
            //if size <= RESPONSE_MAX_UNCHUNKED_SIZE idf will set correct size by itself
            resp.getHeaders().contentLength(addressEnd - addressStart - (ending == (int)endings::TEXT ? 1 : 0));
        }
#ifdef SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER
		resp.getHeaders().connection("close");
#endif

		resp << *this;
		
		return (esp_err_t)ESP_OK;
	}
				
		
	handlerRes file::handle(request& req, response& resp, server& serv) {
		return (esp_err_t)ESP_OK;
	}

	int file::type() const {
		return (int)map::MEMORY;
	}
	
	response& operator<<(response& stream, const file& f) {
		
		ssize_t size =	(ssize_t)(f.addressEnd - f.addressStart);
		
		info("dumping memory file: ", f.name, " ", (void*)f.addressStart, " ", (void*)f.addressEnd, " sizeof ", size);
		
		if (f.ending == (int)endings::TEXT) {
			stream.write((const char*)f.addressStart);
		} else {
			stream.write((const uint8_t*)f.addressStart, f.addressEnd - f.addressStart);
		}		
	
		return stream;
	}
}
