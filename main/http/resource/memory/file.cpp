
#include "file.h"
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
		
		resp.contentType(this->contentType);

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
			stream.writeResp((const char*)f.addressStart);
		} else {
			stream.writeResp((const uint8_t*)f.addressStart, f.addressEnd - f.addressStart);
		}		
	
		return stream;
	}
}
