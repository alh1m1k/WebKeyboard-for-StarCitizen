#pragma once

#include <exception>
#include <algorithm>
#include <cstring>
#include <esp_err.h>
	
	
class not_impleneted: public std::exception {
	
	char 	  _what[20] = {}; 
		
	public:
		not_impleneted(const char* what = "not implemented") {
			memcpy(_what, what, std::min((size_t)19, strlen(what)));
			_what[19] = 0;
		}
		
		virtual const char* what() const noexcept {
			return _what;
		}
		
		virtual esp_err_t code() const noexcept {
			return ESP_FAIL;
		}
};