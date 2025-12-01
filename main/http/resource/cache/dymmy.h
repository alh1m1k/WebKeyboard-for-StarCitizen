#pragma once

#include "file.h"
#include "result.h"
#include "cacheControl.h"

namespace http::resource::cache {

	class dummy {
		public:
			result<codes> operator()(const memory::file& file, request& req, response& resp, server& serv) const noexcept {
				return ESP_OK;
			}
	};

}