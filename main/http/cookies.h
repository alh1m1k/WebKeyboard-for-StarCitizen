#pragma once

#include <string>
#include <forward_list>

#include "esp_http_server.h"

#include "result.h"
#include "cookie.h"


namespace http {
	
	class cookies {

        const httpd_req_t* _handler;

		//todo make it prt ie std::unique_prt<std::forward_list<std::string>> = nullptr;
        std::forward_list<std::string> preserve = {};
		
		public:

            static constexpr uint16_t MINIMUM_ALLOCATED_BYTES = 100; //for sid
		
			cookies();

            explicit cookies(const httpd_req_t* esp_req);

            result<const cookie> get(const std::string& key, uint16_t expectedSize = MINIMUM_ALLOCATED_BYTES);

            result<const cookie> get(const std::string& key, uint16_t expectedSize = MINIMUM_ALLOCATED_BYTES) const;

            resBool set(const cookie& cook);

            resBool set(std::string&& entry);
			
	};
	
}
