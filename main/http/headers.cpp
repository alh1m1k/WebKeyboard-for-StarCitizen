#include "headers.h"
#include "generated.h"

#include "not_implemented.h"

namespace http {

	headers::headers(httpd_req_t* handler) noexcept : handler(handler) {}
	
	bool headers::has(const char* headerId) const noexcept {
		char trash[16];
		return httpd_req_get_hdr_value_str(handler, headerId, trash, 10) != ESP_ERR_NOT_FOUND;
	}
						
	result<std::string> headers::getResult(const char* headerId) const noexcept {
		return get(headerId);
	}

    result<std::string> headers::getResult(const char* headerId) noexcept {
        return get(headerId);
    }
	
	std::string headers::get(const char* headerId) const noexcept {
					
			size_t size = httpd_req_get_hdr_value_len(handler, headerId);
			
			debugIf(LOG_HTTP, "headers::get", headerId, size);
			
			if (size == 0) {
				return {};
			}
			
			std::string ret = std::string("", size); //c++11 will handle null at end of str
			if (auto code = httpd_req_get_hdr_value_str(handler, headerId, ret.data(), size); code != ESP_OK) {
				return {};
			} else {
				return ret;
			}
			
	}

	std::string headers::get(const char* headerId) {
		throw not_impleneted("response headers read");
	}

	resBool headers::set(const char* headerId, const char* str) noexcept {

        if (auto code = httpd_resp_set_hdr(handler, headerId, str); code != ESP_OK) {
            return code;
        } else {
            return ResBoolOK;
        }

	}
	
	const uri& headers::getUri() const {
		if (_uri == nullptr) {
			_uri = std::make_unique<uri>(handler);
		}
		return *_uri;
	}

	uri& headers::getUri() {
		if (_uri == nullptr) {
			_uri = std::make_unique<uri>(handler);
		}
		return *_uri;
	}
	
	const cookies& headers::getCookies() const {
		if (_cookies == nullptr) {
			_cookies = std::make_unique<cookies>(handler);
		}
		return *_cookies;
	}

	cookies& headers::getCookies() {
		if (_cookies == nullptr) {
			_cookies = std::make_unique<cookies>(handler);
		}
		return *_cookies;
	}

}