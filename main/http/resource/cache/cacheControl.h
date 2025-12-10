#pragma once

#include "result.h"
#include "request.h"
#include "response.h"
#include "server.h"

namespace http::resource::cache {

	class cacheControl {
		const char* token;
		public:
			constexpr explicit cacheControl(const std::string& value) noexcept : token(value.c_str()) {};
			result<codes> operator()(request& req, response& resp, server& serv) noexcept {
				resp.getHeaders().cacheControl(token);
				return ESP_OK;
			}
	};

}