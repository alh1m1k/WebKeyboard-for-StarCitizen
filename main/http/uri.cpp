#include "uri.h"

#include "util.h"

namespace http {
		
	uri::uri(){};
	
	uri::uri(const char* str): httpparser::UrlParser(std::string(str))  {};
	
	uri::uri(std::string str): httpparser::UrlParser(str) {};
				
	uri::uri(httpd_req_t* handler, const char* schema): httpparser::UrlParser(true){
		
		size_t querySize 	= strlen(handler->uri);
		size_t schemaSize 	= strlen(schema);
		
		if (size_t hostSize = httpd_req_get_hdr_value_len(handler, "Host"); hostSize > 0) {
			char   buffer[querySize+hostSize+1+schemaSize];
			buffer[0] = 0;
			strcat(buffer, schema);
			if (auto code = httpd_req_get_hdr_value_str(handler, "Host", &buffer[schemaSize], hostSize+1); code != ESP_OK) {
				debugIf(LOG_HTTP, "uri::uri::httpd_req_t unable retrieve host", code);
				parse(std::string(handler->uri));
			} else {
				
				if (strlen(buffer) == hostSize) {
					debugIf(LOG_HTTP, "api is null based", hostSize);
				} else {
					debugIf(LOG_HTTP, "api is size based", hostSize);
				}
				
				if (hostSize > 0) {
					if (buffer[hostSize-1] == '/') {
						buffer[hostSize-1] = 0;
					}
				}
				if (querySize > 1) {
					strcat(buffer, handler->uri);
				}
				debugIf(LOG_HTTP, "rebuilded uri is", &buffer[0]);
				bool valid =parse(std::string(buffer));
				debugIf(LOG_HTTP, "parce res", &buffer[0]);
			}
		} else {
			if (querySize > 1) {
				char   buffer[querySize+4+1];
				strcat(buffer, "host");
				strcat(buffer, handler->uri);
				parse(std::string(buffer));
			} else {
				parse(std::string("host"));
			}
		}
		debugIf(LOG_HTTP, "uri::uri::httpd_req_t", handler->uri);		
			
	};

}