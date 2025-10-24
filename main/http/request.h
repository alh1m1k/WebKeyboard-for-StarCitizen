#pragma once

#include <cstddef>
#include  <memory>

#include <esp_http_server.h>

#include "../exception/not_implemented.h"
#include "parsing_error.h"

#include "uri.h"
#include "cookies.h"
#include "headers.h"
#include "http/session/interfaces/iSession.h"
#include "http/session/pointer.h"

namespace http {

	class cookies;
	class headers;
	class action;

	class request {
						
		httpd_req_t* handler;
		
		mutable std::unique_ptr<headers> _headers;
		
		action& 	_route;
		
		public:
		
			//explicit request(httpd_req_t* esp_req);
			explicit request(httpd_req_t* esp_req, action& route);
						
			const headers& getHeaders() const;
			
			const uri& getUri() const;

            httpd_method_t getMethod() const;

            inline bool isGet() const {
                return getMethod() == httpd_method_t::HTTP_GET;
            }

            inline bool isPost() const {
                return getMethod() == httpd_method_t::HTTP_POST;
            }
			
			const char* getUriRaw() const; //todo remove me after proper implementing defered object 
			
			const cookies& getCookies() const;
			
			inline const action& getRoute() const {
                return _route;
            }

            std::shared_ptr<session::iSession> getSession() const;

			//method needed to expose private handler in order to successfuly build sub object like sockets or headers
			//without using friend decl as it make linking impossible to manage
			inline httpd_req_t* native() const {
				return handler;
			}
			
	};

    template<class T>
    inline std::shared_ptr<T> session_acquire(request& r) noexcept {
        return session::acquire<T>(*static_cast<session::pointer*>(r.native()->sess_ctx));
    }
}

