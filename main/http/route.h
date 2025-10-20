#pragma once

#include "generated.h"

#include <algorithm>
#include <cstring>
#include <functional>

#include "esp_err.h"

#include "request.h"
#include "response.h"
#include "server.h"
#include "socket/handler.h"


//class represent memory slot for path, callback and esp_handler
namespace http {

    namespace socket {
        class handler;
    }
	
	class server;

	class route {
        public:

            typedef result<codes> handler_res_type;
            typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;

        private:

            friend server;

            char * 			c_str    	= nullptr;
            httpd_uri 		esp_handler = {};
            handler_type    _callback;
            server&         _owner;
				
		public:

			route(std::string_view& path, httpd_method_t mode, handler_type _callback, server& owner);
			
			route(route& copy);
			
			route(route&& move);
			
			virtual ~route();
			
			route& operator=(const route& copy);
			
			route& operator=(route&& move);
			
			bool operator==(const route& other) const;
			
			bool operator!=(const route& other) const;

			bool samePath(const route& other) const;

			handler_res_type operator()(request& req, response& resp);
			
			inline std::string_view path() const {
				return std::string_view(c_str);
			}
			
			inline httpd_method_t mode() const {
				return esp_handler.method;
			}

            inline bool isWebSocket() const {
                return _callback.target<http::socket::handler>();
            }
			
			inline const handler_type& callback() const {
				return _callback;
			}

            inline server& owner() const {
                return _owner;
            }
	};
}

