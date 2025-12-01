#pragma once

#include "file.h"
#include "result.h"

namespace http::resource::cache {

	class eTag {
		public:
			result<codes> operator()(const memory::file& file, request& req, response& resp, server& serv) const noexcept {

				if (file.checksum != nullptr) {
					if (req.getHeaders().ifNoneMatch() == file.checksum) {
						return codes::NOT_MODIFIED;
					} else {
						resp.getHeaders().eTag(file.checksum);
					}
				}

				return ESP_OK;
			}
	};

}