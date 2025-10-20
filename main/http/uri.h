#pragma once

#include "generated.h"

#include <cstring>

#include "esp_http_server.h"
#include "parsing_error.h"

#include "urlparser.h"


namespace http {
	class uri: public httpparser::UrlParser {
		public:
		
			uri();
			
			explicit uri(const char* str);
			
			explicit uri(std::string str);
						
			explicit uri(httpd_req_t* handler, const char* schema = "http://");
			
	};
}