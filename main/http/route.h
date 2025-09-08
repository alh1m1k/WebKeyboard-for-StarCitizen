#pragma once

#include "generated.h"

#include "esp_err.h"
#include <algorithm>
#include <cstring>
#include "response.h"
#include "server.h"
#include "handler.h"


//class represent memory slot for path, callback and esp_handler
namespace http {
	
	class server;
	
	typedef result<codes> handlerRes;
	typedef std::function<handlerRes(request& req, response& resp)> handler;
		
	class route {
		
		friend server;
		
		char * 			c_str    	= nullptr;
		httpd_uri 		esp_handler = {};
		handler			_callback;
				
		public:
				
			route(std::string_view& path, httpd_method_t mode, handler _callback);
			
			route(route& copy);
			
			route(route&& move);
			
			virtual ~route();
			
			route& operator=(const route& copy);
			
			route& operator=(route&& move);
			
			bool operator==(const route& other) const;
			
			bool operator!=(const route& other) const;
			
			bool samePath(const route& other) const;
			
			handlerRes operator()(request& req, response& resp);
			
			std::string_view path() const {
				return std::string_view(c_str);
			}
			
			httpd_method_t mode() const {
				return esp_handler.method;
			}
			
			const handler& callback() const {
				return _callback;
			}
	};
}

