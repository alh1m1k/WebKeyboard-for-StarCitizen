#pragma once

#include "generated.h"

#include <esp_http_server.h>
#include <deque>
#include <sys/_stdint.h>
#include <mutex>

#include "response.h"
#include "request.h"
#include "../result.h"
#include "handler.h"
#include "route.h"


namespace http {
			
	class server {
		
		httpd_handle_t  handler = {};
		//httpd_config    config = HTTPD_DEFAULT_CONFIG();
				
		struct {
			std::deque<class route> data = {}; //std::deque in order to protect from ptr invalidataion that kill later deref via c handlers
			std::mutex m = {};
		} routes;
		
		public:
		
			server();
			
			server(server&) = delete;
			
			virtual ~server();
			
			auto operator=(server&) = delete;
			
			resBool begin(uint16_t port);
			
			resBool end();
			
			resBool addHandler(std::string_view path, httpd_method_t mode, http::handler callback);
			
			resBool removeHandler(std::string_view path, httpd_method_t mode);
			
			bool    hasHandler(std::string_view path, httpd_method_t mode);
			
			//resBool addRouter (std::string_view path = "/", http::handler callback);
		
	};
}


