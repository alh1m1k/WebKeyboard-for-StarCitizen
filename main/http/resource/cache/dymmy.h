#pragma once

#include "result.h"
#include "request.h"
#include "response.h"
#include "server.h"

namespace http::resource::cache {

	class dummy {
		public:
			result<codes> operator()(request& req, response& resp, server& serv) noexcept {
				return ESP_OK;
			}
	};

}