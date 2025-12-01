#include "file.h"
#include "generated.h"


#include "contentType.h"
#include "resource.h"
#include "util.h"
#include <sys/_stdint.h>

namespace http::resource::memory {

	//file::file(int addressStart, int addressEnd, endings end, const char * name) : addressStart(addressStart), addressEnd(addressEnd), ending((int)end), name(name) {};

	const file::cache_control_type file::noControl = {};

	file::file(int addressStart, int addressEnd, endings end, const char * name, const char * contentType, const char* checksum, const cache_control_type& cache) noexcept
		: addressStart(addressStart), addressEnd(addressEnd), ending((int)end),
		  name(name), contentType(contentType), cacheControl(cache), checksum(checksum) {};
		
	file::file(int addressStart, int addressEnd, endings end, const char * name, enum contentType ct, const char* checksum, const cache_control_type& cache) noexcept
		: addressStart(addressStart), addressEnd(addressEnd), ending((int)end),
		  name(name), contentType(contentType2Symbols(ct)), cacheControl(cache), checksum(checksum) {};
	
	bool file::operator==(const file& other) const noexcept {
		return other.name == this->name;
	}
	
	bool file::operator!=(const file& other) const noexcept {
		return other.name != this->name;
	}
	
	bool file::operator<(const file& other) const noexcept {
		return other.name < this->name;
	}

	resource::handler_res_type file::operator()(request& req, response& resp, server& serv) const noexcept {

		ssize_t size = addressEnd - addressStart;
		//headers
		if constexpr (SOCKET_RECYCLE_CLOSE_RESOURCE_REQ_VIA_HTTP_HEADER) {
			resp.getHeaders().connection("close");
		}

		if (auto result = cacheControl(*this, req, resp, serv); !result) {
			return ESP_FAIL;
		} else if (std::holds_alternative<codes>(result)) {
			return std::get<codes>(result);
		}

		resp.getHeaders().contentType(this->contentType);

		if constexpr (RESOURCE_COMPRESSION) {
			if (req.getHeaders().acceptEncoding().contains("gzip")) {
				resp.getHeaders().contentEncoding("gzip");
			} else {
				//decompressor interface  { std::span<uin8_t> decompress() }
				//attachDecompressorBuilder()
				//getDecompressor(*this, MEMORY_LIMIT)
				// while(auto nextBuffer = decompressor.decompress(); nextBuffer != decompressor::eof ) {
				// resp << nextBuffer;
				// }
				//or refactor file to template
				error("fixme", "runtime decompression", " ", req.getHeaders().acceptEncoding());
				return codes::UNSUPPORTED_MEDIA;
			}
		}

        if (size > RESPONSE_MAX_UNCHUNKED_SIZE) {
            resp.getHeaders().contentLength(addressEnd - addressStart - (ending == (int)endings::TEXT ? 1 : 0));
		} else {
			//if size <= RESPONSE_MAX_UNCHUNKED_SIZE idf will set correct size by itself
		}

		resp << *this;

		return (esp_err_t)ESP_OK;
	}
				
		
	resource::handler_res_type file::handle(request& req, response& resp, server& serv) const noexcept {
		return (esp_err_t)ESP_OK;
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
