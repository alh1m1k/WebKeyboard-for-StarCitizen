#pragma once

#include "generated.h"

#include <mutex>
#include <memory>

#include <esp_http_server.h>


#include "result.h"
#include "request.h"
#include "response.h"
#include "action.h"
#include "http/session/interfaces/iManager.h"

namespace http {

    namespace session {
        class iManager;
    }

	//im fail to declare {anonimus}::captiveOf
	action& captiveOf(server& serv) noexcept;

	class server {

		friend action& captiveOf(server& serv) noexcept;

        mutable std::mutex _m;

        mutable std::unique_ptr<session::iManager> _sessions;

        protected:

			httpd_handle_t  handler = {};

			action captive;

			//allow to hook from descendants
			static esp_err_t socketOpen(httpd_handle_t hd, int sockfd) noexcept;
			static void 	 socketClose(httpd_handle_t hd, int sockfd) noexcept;
			static void 	 globalUserCtxFree(void*) noexcept;
			httpd_config_t 	 config(uint16_t port) noexcept;
			void 			 afterStart() noexcept;
			void 			 beforeStop() noexcept;

            void setSessions(std::unique_ptr<session::iManager>&& manager);

		public:

			typedef result<codes> handler_res_type;
            typedef std::function<handler_res_type(request& req, response& resp, server& serv)> handler_type;
            typedef std::unique_ptr<session::iManager> sessions_ptr_type;
			typedef std::function<void()> job_type;

			static server* of(httpd_handle_t handler);


			server() noexcept;

			server(const server&) = delete;

            virtual ~server() = default;

			auto operator=(const server&) = delete;

            inline httpd_handle_t native() noexcept {
                return handler;
            }
			
			resBool begin(uint16_t port = 80) noexcept;
			
			resBool end() noexcept;
			
			resBool addHandler(const char * url, httpd_method_t mode, handler_type&& callback);
			
			resBool removeHandler(const char * url, httpd_method_t mode);
			
			bool    hasHandler(const char * url, httpd_method_t mode) noexcept;

			void 	captiveHandler(handler_type&& callback);

			void 	collect();

            template<class TSessionManager, typename... Args>
            inline void attachSessions(Args&&... args) { //todo init vector
                setSessions(std::make_unique<TSessionManager>(std::forward<Args>(args)...));
            }

			resBool scheduleJob(job_type&& job);


            const sessions_ptr_type& getSessions() const;

	};


}


