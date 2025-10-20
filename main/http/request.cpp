#include "request.h"

#include "session/pointer.h"

namespace http {
			
	//request::request(httpd_req_t* esp_req) : handler(esp_req) {}
	request::request(httpd_req_t* esp_req, route& route) : handler(esp_req), _route(route) {}
				
	const headers& request::getHeaders() const {
		
		if (_headers == nullptr) {
			_headers = std::make_unique<headers>(handler);
		}
		
		return *_headers;		
	}

    std::shared_ptr<session::iSession> request::getSession() const {
        if (handler->sess_ctx != nullptr) {
            return static_cast<http::session::pointer*>(handler->sess_ctx)->lock();
        } else {
            return nullptr;
        }
    }
	
	const uri& request::getUri() const {
        return getHeaders().getUri();
	}
	
	const char* request::getUriRaw() const {
		return handler->uri;
	}
	
	const cookies& request::getCookies() const {
        return getHeaders().getCookies();
	}

	httpd_method_t request::getMethod() const {
		return (httpd_method_t)handler->method;
	} 	
}

