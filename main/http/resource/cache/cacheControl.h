#pragma once

#include "file.h"
#include "result.h"

namespace http::resource::cache {

	class cacheControl {
		std::string token;
		public:
			cacheControl(const std::string& value) noexcept : token(value) {};
			result<codes> operator()(const memory::file& file, request& req, response& resp, server& serv) const noexcept {
				resp.getHeaders().cacheControl(token);
				return ESP_OK;
			}
	};

}