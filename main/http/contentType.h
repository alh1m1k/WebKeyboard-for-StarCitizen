#pragma once

#include "assert.h"


#define HTTPD_TYPE_JSON   "application/json"            /*!< HTTP Content type JSON */
#define HTTPD_TYPE_TEXT   "text/html"                   /*!< HTTP Content type text/HTML */
#define HTTPD_TYPE_OCTET  "application/octet-stream"    /*!< HTTP Content type octext-stream */

namespace http {
	
	enum class contentType {
		JSON,
		TEXT,
		OCTET,
		JS,
		CSS,
		SVG,
	};
		
	[[maybe_unused]] static const char* contentType2Symbols (const contentType& code) {
	       switch (code) {
            case contentType::JSON: 			return "application/json";
			case contentType::TEXT: 			return "text/html";
			case contentType::OCTET:  			return "application/octet-stream";
			case contentType::JS:  				return "application/javascript";
			case contentType::CSS:  			return "text/css";
			case contentType::SVG:				return "image/svg+xml";
            default:
                assert("contentType2Symbols no default value");
                return nullptr;
        }
	}
	
	
} 