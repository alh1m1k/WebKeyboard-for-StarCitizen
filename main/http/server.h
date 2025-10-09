#pragma once

#include "generated.h"

#include <esp_http_server.h>
#include <deque>
#include <sys/_stdint.h>
#include <mutex>
#include <memory>

#include "response.h"
#include "request.h"
#include "../result.h"
#include "handler.h"
#include "route.h"
#include "session/baseManager.h"


namespace http {

    namespace session {
        class baseManager;
    }
			
	class server {

		httpd_handle_t  handler = {};
		//httpd_config    config = HTTPD_DEFAULT_CONFIG();
				
		struct {
			std::deque<class route> data = {}; //std::deque in order to protect from ptr invalidataion that kill later deref via c handlers
			std::mutex m = {};
		} routes;

        mutable std::unique_ptr<session::baseManager> _session;
		
		public:
		
			server();
			
			server(server&) = delete;
			
			virtual ~server();
			
			auto operator=(server&) = delete;
			
			resBool begin(uint16_t port);
			
			resBool end();
			
			resBool addHandler(std::string_view path, httpd_method_t mode, http::handler callback);
			
			resBool removeHandler(std::string_view path, httpd_method_t mode);
			
			bool    hasHandler(std::string_view path, httpd_method_t mode);

            session::baseManager& session();
		
	};
}


