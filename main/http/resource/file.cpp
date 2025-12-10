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

		if (auto result = callback(req, resp, serv, *this); !result) {
			return ESP_FAIL;
		} else if (std::holds_alternative<codes>(result)) {
			return std::get<codes>(result);
		}

		ssize_t size = addressEnd - addressStart;

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

	response& operator<<(response& stream, const file& f) {

		info("dumping memory file: ", f.name, " sizeof ", f.addressEnd - f.addressStart);
		
		if (f.ending == (int)endings::TEXT) {
			stream.write((const char*)f.addressStart);
		} else {
			stream.write((const uint8_t*)f.addressStart, f.addressEnd - f.addressStart);
		}		
	
		return stream;
	}
}
