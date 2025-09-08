#pragma once

#include "esp_http_server.h"

namespace http {
	
	class cookies {
		
		public:
		
			cookies();
			
			explicit cookies(const char* str);
			
			//explicit cookies(std::string str) {};
			
			explicit cookies(const httpd_req_t* esp_req);
			
	};
	
}
