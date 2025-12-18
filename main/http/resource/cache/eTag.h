#pragma once

#include "result.h"
#include "request.h"
#include "response.h"
#include "server.h"

namespace http::resource::cache {

	class eTag {
		const char* tag;
		public:
			constexpr explicit eTag(const char* tag) : tag(tag) {};
			result<codes> operator()(request& req, response& resp, server& serv) noexcept {
				if (tag != nullptr) {
					if (req.getHeaders().ifNoneMatch() == tag) {
						return codes::NOT_MODIFIED;
					} else {
						resp.getHeaders().c_eTag(tag);
					}
				}
				return ESP_OK;
			}
	};

}