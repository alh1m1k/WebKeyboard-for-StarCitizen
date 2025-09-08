#include "request.h"

namespace http {
			
	request::request(httpd_req_t* esp_req) : handler(esp_req) {}
	request::request(httpd_req_t* esp_req, route* route) : handler(esp_req), _route(route) {}
				
	headers& request::getHeaders() {
		
		if (_headers == nullptr) {
			
			_headers = std::make_unique<headers>(handler);
			
		}
		
		return *_headers;		
	}
	
	uri& request::getUri() {
		return getHeaders().getUri();
	}
	
	const char* request::getUriRaw() const {
		return handler->uri;
	}
	
	cookies& request::getCookies() {
		return getHeaders().getCookies();
	}
	
	const route* request::getRoute() {
		return _route;
	} 
	
	httpd_method_t request::getMethod() const {
		return (httpd_method_t)handler->method;
	} 	
}

