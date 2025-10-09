#include "request.h"

namespace http {
			
	//request::request(httpd_req_t* esp_req) : handler(esp_req) {}
	request::request(httpd_req_t* esp_req, route& route) : handler(esp_req), _route(route) {}
				
	const headers& request::getHeaders() {
		
		if (_headers == nullptr) {
			_headers = std::make_unique<headers>(handler);
		}
		
		return *_headers;		
	}

    session::baseSession* request::getSession() const {
        if (auto result = _route.owner().session().open(); result) {
            return std::get<session::baseSession*>(result);
        } else {
            return nullptr;
        }
    }
	
	const uri& request::getUri() {
        getHeaders();
		return _headers->getUri();
	}
	
	const char* request::getUriRaw() const {
		return handler->uri;
	}
	
	const cookies& request::getCookies() {
        getHeaders();
		return _headers->getCookies();
	}

	httpd_method_t request::getMethod() const {
		return (httpd_method_t)handler->method;
	} 	
}

