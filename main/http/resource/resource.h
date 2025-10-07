#pragma once

#include "generated.h"

#include "../handler.h"
#include "esp_err.h"

namespace http::resource {
	
	class resource {

		protected:
			virtual handlerRes handle(request& req, response& resp, server& serv) {
				return (esp_err_t)ESP_OK;
			}
		
		public: 
				
			virtual int type() const = 0;

            virtual ~resource() = default;
			
			handlerRes operator()(request& req, response& resp, server& serv) {
				debugIf(LOG_HTTP, "http::resource::resource", (void*)this);
				return this->handle(req, resp, serv);
			}
	};
	
}
