#pragma once

#include "generated.h"

#include <cstring>

#include "esp_http_server.h"
#include "parsing_error.h"

namespace http {
	class uri {
		public:
			uri();
			uri(httpd_req_t* handler);
	};
}