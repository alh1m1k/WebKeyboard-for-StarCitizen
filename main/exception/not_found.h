#pragma once

#include <exception>
#include <cstring>
#include <functional>

#include <esp_err.h>
	
	
class not_found: public std::bad_function_call {
	
	char 	  _what[20] = {}; 
	
	public:
		not_found(const char* what) {
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

