#include "file.h"
#include "generated.h"

#include "contentType.h"
#include "util.h"

namespace http::resource {

	file::file(
		int addressStart,
		int addressEnd,
		endings end,
		const char* name,
		const char* contentType,
		const char* checksum,
		const callback_type& callback
	) noexcept :
		addressStart(addressStart),
		addressEnd(addressEnd),
		ending((int)end),
		name(name),
		contentType(contentType),
		checksum(checksum),
		callback(callback)
	{};
		
	file::file(
		int addressStart,
		int addressEnd,
		endings end,
		const char* name,
		enum contentType ct,
		const char* checksum,
		const callback_type& callback
	) noexcept :
		addressStart(addressStart),
		addressEnd(addressEnd),
		ending((int)end),
		name(name),
		contentType(contentType2Symbols(ct)),
		checksum(checksum),
		callback(callback)
	{};

	file::handler_res_type file::operator()(request& req, response& resp, server& serv) const noexcept {
		//headers

		if (callback != nullptr) {
			if (auto result = callback(req, resp, serv, *this); !result) {
				return ESP_FAIL;
			} else if (std::holds_alternative<codes>(result)) {
				return std::get<codes>(result);
			}
		}

		resp.getHeaders().contentType(contentType);

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
				error("fixme", "runtime decompression", " ", req.getUri().raw(), " ", req.getHeaders().acceptEncoding());
				return codes::UNSUPPORTED_MEDIA;
			}
		}

		//"Content-Length and Transfer-Encoding must not be set together or Content-Length must be unset"
		//it seems esp.idf internal send_fn is implicit split buffer in to chunks(but not sure how), so much of socket.cpp
		//is reimplemented by me, in future it must be dropped for cleaner code
		//also Content-Length is internally set to appropriate value
		//if this data is transferred in full or droped if transferred in chunks.
		//HTTP/1.1 specification, RFC 2616, Section 4.4, Point 3

/*
		//no need
		ssize_t size = addressEnd - addressStart;
        if (size <= RESPONSE_MAX_UNCHUNKED_SIZE) {
			resp.getHeaders().contentLength(size - (ending == (int)endings::TEXT ? 1 : 0));
		}
*/

		resp << *this;

		return (esp_err_t)ESP_OK;
	}

	response& operator<<(response& stream, const file& f) {

		ssize_t size = f.addressEnd - f.addressStart;
		info("dumping memory file:", f.name, " sizeof ", size);
		
		if (f.ending == (int)endings::TEXT) {
			stream.write((const uint8_t*)f.addressStart, size - 1);
		} else {
			stream.write((const uint8_t*)f.addressStart, f.addressEnd - f.addressStart);
		}

		return stream;
	}
}
