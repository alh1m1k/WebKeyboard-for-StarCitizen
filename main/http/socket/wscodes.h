#pragma once

#include "assert.h"

namespace http::socket {

	//name it as wscodes to preserve consistency with http:codes as come
	//http:socket already use http:code by name `codes`
	enum class wscodes: int16_t {
		NORMAL_CLOSE	 = 1000,
		AWAY			 = 1001,
		UNSUPPORTED_DATA = 1003,
		INTERNAL_ERROR   = 1011,
		TRY_AGAIN		 = 1013,

		SOCKET_TIMEOUT   = 3001,
		SESSION_CLOSED   = 3002,
		UNAUTHORIZED     = 3003
	};
		
	[[maybe_unused]] static const char* codes2Symbols (const wscodes& code) {
	       switch (code) {
			case wscodes::NORMAL_CLOSE:
			case wscodes::AWAY:
			case wscodes::UNSUPPORTED_DATA:
			case wscodes::INTERNAL_ERROR:
			case wscodes::TRY_AGAIN:
	       		return "";
		    case wscodes::SOCKET_TIMEOUT:			return "3001 Timeout";
            case wscodes::SESSION_CLOSED: 		return "3002 Session closed";
			case wscodes::UNAUTHORIZED: 			return "3003 Unauthorized";
            default:
                assert(false && "codes2Symbols no default value");
                return nullptr;
        }
	}
	
	
} 