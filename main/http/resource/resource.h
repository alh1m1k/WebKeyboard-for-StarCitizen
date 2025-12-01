#pragma once

#include "generated.h"

#include <functional>

#include "result.h"
#include "esp_err.h"
#include "request.h"
#include "response.h"
#include "server.h"

namespace http::resource {

	class resource {

		protected:
			virtual result<codes> handle(request& req, response& resp, server& serv) const noexcept {
				return (esp_err_t)ESP_OK;
			}
		
		public:

			typedef result<codes> handler_res_type;
			typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;

            virtual ~resource() = default;
			
			handler_res_type operator()(request& req, response& resp, server& serv) const noexcept {
				debugIf(LOG_HTTP, "http::resource::resource", (void*)this);
				return this->handle(req, resp, serv);
			}
	};
	
}
