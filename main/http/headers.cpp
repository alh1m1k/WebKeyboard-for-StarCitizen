#include "headers.h"
#include "generated.h"

namespace http {
	
		
	headers::headers() {
		
	}
	
	headers::headers(httpd_req_t* handler): handler(handler), populated(true)  {}
	
	bool headers::has(const char* headerId) const {
		char trash[16];
		return httpd_req_get_hdr_value_str(handler, headerId, trash, 10) != ESP_ERR_NOT_FOUND;
								
	}
						
	result<std::string> headers::getResult(const char* headerId) const {
				
			size_t size = httpd_req_get_hdr_value_len(handler, headerId);
			
			debugIf(LOG_HTTP, "headers::getResut", headerId, size);
			
			if (size == 0) {
				return (esp_err_t)ESP_FAIL;
			}
			
			std::string ret = std::string("", size);
			
			if (auto code = httpd_req_get_hdr_value_str(handler, headerId, ret.data(), size); code != ESP_OK) {
				return (esp_err_t)ESP_FAIL;
			} else {
				return ret;
			}
			
	}
	
	std::string headers::get(const char* headerId) const {
					
			size_t size = httpd_req_get_hdr_value_len(handler, headerId);
			
			debugIf(LOG_HTTP, "headers::get", headerId, size);
			
			if (size == 0) {
				return std::string("");
			}
			
			std::string ret = std::string("", size); //c++11 will handle null at end of str
			
			if (auto code = httpd_req_get_hdr_value_str(handler, headerId, ret.data(), size); code != ESP_OK) {
				return std::string();
			} else {
				return ret;
			}
			
	}
							
	//no way to implement fast buffer, size API
							
	resBool headers::set(const char* headerId, const char* str) const {

			if (auto code = httpd_resp_set_hdr(handler, headerId, str); code != ESP_OK) {
				return code;
			} else {
				return ResBoolOK;
			}
			
	}
	
	uri& headers::getUri() {
		
		if (_uri == nullptr) {
			if (populated) {
				_uri = std::make_unique<uri>(handler);
			} else {
				_uri = std::make_unique<uri>();
			}			
		}
		
		return *_uri;
	}
	
	cookies& headers::getCookies() {
		
		if (_cookies == nullptr) {
			if (populated) {
				_cookies = std::make_unique<cookies>(handler);
			} else {
				_cookies = std::make_unique<cookies>();
			}			
		}
		
		return *_cookies;
	}
				
	resBool headers::contentEncoding(std::string& str) const {
		return set("content-encoding", str.c_str());
	}
	
	resBool headers::contentEncoding(std::string&& str) const {
		return set("content-encoding", str.c_str());
	}
	
	resBool headers::contentLength(std::string& str) const {
		return set("content-length", str.c_str());
	}
	
	resBool headers::contentLength(std::string&& str) const {
		return set("content-length", str.c_str());
	}
	
	resBool headers::contentType(std::string& str) const {
		return set("content-type", str.c_str());
	}
	
	resBool headers::contentType(std::string&& str) const {
		return set("content-type", str.c_str());
	}
	
	resBool headers::server(std::string& str) const {
		return set("server", str.c_str());
	}
	
	resBool headers::server(std::string&& str) const {
		return set("server", str.c_str());
	}
	
	resBool headers::setCookies(std::string& str) const {
		return set("set-server", str.c_str());
	}
	
	resBool headers::setCookies(std::string&& str) const {
		return set("set-server", str.c_str());
	}	
		
}