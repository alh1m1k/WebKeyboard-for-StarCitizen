#pragma once

#include  <exception>
#include <esp_err.h>
#include <esp_http_server.h>


#include "errors.h"

	
class invalid_descriptor: public std::exception {
	
	const char buffer[30+20] = "fd invalid: ";
	
	public:
	
		invalid_descriptor(const char* when = "not specified") {
			strlcat((char*)&buffer[0], when, 30+20);
		}
		
		virtual const char* what() const noexcept {
			return buffer;
		}
		
		virtual esp_err_t code() const noexcept {
			return HTTP_ERR_FD_INVALID;
		}
};