#pragma once

#include <exception>
#include <cstring>
#include <functional>

#include <esp_err.h>
	
	
class bad_api_call: public std::bad_function_call {
	
	char 	  _what[20] = {}; 
	esp_err_t _code = ESP_FAIL;
	
	public:
		bad_api_call(const char* what, esp_err_t code): _code(code) {
			memcpy(_what, what, std::min((size_t)19, strlen(what)));
			_what[19] = 0;
		}
		
		virtual const char* what() const noexcept {
			return _what;
		}
		
		virtual esp_err_t code() const noexcept {
			return _code;
		}
};