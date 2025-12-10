#pragma once

#define HTTPD_TYPE_JSON   "application/json"            /*!< HTTP Content type JSON */
#define HTTPD_TYPE_TEXT   "text/html"                   /*!< HTTP Content type text/HTML */
#define HTTPD_TYPE_OCTET  "application/octet-stream"    /*!< HTTP Content type octext-stream */

namespace http {
	
	enum class contentType {
		JSON,
		TEXT,
		HTML,
		OCTET,
		JS,
		CSS,
		SVG,
		UNDEFINED,
	};
		
	[[maybe_unused]] static const char* contentType2Symbols (const contentType& code) {
	       switch (code) {
            case contentType::JSON: 			return "application/json";
			case contentType::TEXT: 			return "text/plain";
			case contentType::HTML: 			return "text/html";
			case contentType::OCTET:  			return "application/octet-stream";
			case contentType::JS:  				return "application/javascript";
			case contentType::CSS:  			return "text/css";
			case contentType::SVG:				return "image/svg+xml";
			case contentType::UNDEFINED:		return nullptr;
            default:
                assert("contentType2Symbols no default value");
                return nullptr;
        }
	}
	
	
} 