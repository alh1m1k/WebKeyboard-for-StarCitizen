#pragma once

#include <exception>
#include <algorithm>
#include <cstring>
#include <esp_err.h>
	
	
class parsing_error: public std::exception {
	
	char 	  _what[20] = {}; 
		
	public:
		parsing_error(const char* what = "parsing error") {
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