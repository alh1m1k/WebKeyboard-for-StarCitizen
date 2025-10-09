#pragma once

#include <cstddef>
#include <esp_http_server.h>
#include  <memory>

#include "parsing_error.h"
#include "cookies.h"
#include "headers.h"
#include "../exception/not_implemented.h"
#include "uri.h"
#include "session/baseSession.h"

namespace http {
	
	class response;
	class cookies;
	class headers;
	class route;
			
	class request {
						
		httpd_req_t* handler;
		
		mutable std::unique_ptr<headers> _headers;
		
		route& 	_route;
		
		public:
		
			//explicit request(httpd_req_t* esp_req);
			explicit request(httpd_req_t* esp_req, route& route);
						
			const headers& getHeaders();
			
			const uri& getUri();

            httpd_method_t getMethod() const;

            inline bool isGet() const {
                return getMethod() == httpd_method_t::HTTP_GET;
            }

            inline bool isPost() const {
                return getMethod() == httpd_method_t::HTTP_POST;
            }
			
			const char* getUriRaw() const; //todo remove me after proper implementing defered object 
			
			const cookies& getCookies();
			
			inline const route& getRoute() const {
                return _route;
            }

            session::baseSession* getSession() const;

			//method needed to expose private handler in order to successfuly build sub object like sockets or headers
			//without using friend decl as it make linking impossible to manage
			inline httpd_req_t* native() const {
				return handler;
			}
			
	};
}

