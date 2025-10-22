#pragma once

#include "generated.h"

#include <deque>
#include <sys/_stdint.h>
#include <mutex>
#include <memory>

#include <esp_http_server.h>


#include "result.h"
#include "request.h"
#include "response.h"
#include "route.h"
#include "http/session/interfaces/iManager.h"


namespace http {

    namespace session {
        class iManager;
    }

    //this class became a template
	class server {

		httpd_handle_t  handler = {};

		struct {
			std::deque<class route> data = {}; //std::deque in order to protect from ptr invalidation that kill later deref via c handlers
			mutable std::mutex m = {};
		} routes;

        mutable std::unique_ptr<session::iManager> _sessions;

        protected:

            void setSessions(std::unique_ptr<session::iManager>&& manager);

		public:

            typedef result<codes> handler_res_type;
            typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;
            typedef std::unique_ptr<session::iManager> sessions_ptr_type;


			server() = default;

			server(server&) = delete;

            virtual ~server() = default;

			auto operator=(server&) = delete;

            inline httpd_handle_t native() {
                return handler;
            }
			
			resBool begin(uint16_t port);
			
			resBool end();
			
			resBool addHandler(std::string_view path, httpd_method_t mode, const handler_type& callback);
			
			resBool removeHandler(std::string_view path, httpd_method_t mode);
			
			bool    hasHandler(std::string_view path, httpd_method_t mode);

            template<class TSessionManager>
            inline void attachSessions() { //todo init vector
                setSessions(std::make_unique<TSessionManager>());
            }

            const sessions_ptr_type& getSessions() const;

	};
}


