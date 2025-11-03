#pragma once

#include "assert.h"

namespace http {
	
	enum class codes {
		SWITCHING_PROTOCOLS	    = 101,
		OK 						= 200,
		NO_CONTENT 				= 204,
		MULTI_STATUS 			= 207,
		BAD_REQUEST 			= 400,
		UNAUTHORIZED 			= 401,
		NOT_FOUND 				= 404,
		REQUEST_TIMEOUT 		= 408,
        TOO_MANY_REQUESTS       = 429,
		INTERNAL_SERVER_ERROR 	= 500,
	};
		
	[[maybe_unused]] static const char* codes2Symbols (const codes& code) {
	       switch (code) {
		    case codes::SWITCHING_PROTOCOLS:    return "101 Switching Protocols";
            case codes::OK: 					return "200 OK";
			case codes::NO_CONTENT: 			return "204 No Content";
			case codes::MULTI_STATUS:  			return "207 Multi-Status";
			case codes::BAD_REQUEST:  			return "400 Bad Request";
		    case codes::UNAUTHORIZED:  			return "401 Unauthorized";
			case codes::NOT_FOUND:  			return "404 Not Found";
			case codes::REQUEST_TIMEOUT:  		return "408 Request Timeout";
            case codes::TOO_MANY_REQUESTS:  	return "429 Too Many Requests";
			case codes::INTERNAL_SERVER_ERROR:  return "500 Internal Server Error";
            default:
                assert("codes2Symbols no default value");
                return nullptr;
        }
	}
	
	
} 