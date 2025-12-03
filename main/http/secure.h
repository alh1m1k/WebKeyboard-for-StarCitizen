#pragma once

#include <span>

#include <esp_https_server.h>

#include "server.h"



namespace http {

	class secure : server {

		protected:

			//allow to hook from descendants
			httpd_ssl_config_t config(
				uint16_t port,
				const std::span<const uint8_t>& certPem,
				const std::span<const uint8_t>& privKeyPem,
				const std::span<const uint8_t>& caCertPem
			) 	noexcept;

		public:

			using server::handler_res_type;
			using server::handler_type;
			using server::sessions_ptr_type;
			using server::job_type;

			typedef std::span<const uint8_t> cert_data_type;

			static inline secure* of(httpd_handle_t handler) noexcept {
				return static_cast<secure*>(server::of(handler));
			}

			secure() 						= default;

			secure(const secure&) 			= delete;

			virtual ~secure() 				= default;

			auto operator=(const secure&) 	= delete;

			using server::native;
			using server::addHandler;
			using server::removeHandler;
			using server::hasHandler;
			using server::captiveHandler;
			using server::collect;
			using server::attachSessions;
			using server::scheduleJob;
			using server::getSessions;

			inline resBool begin(uint16_t port) noexcept {
				return begin(port, {}, {}, {});
			}

			resBool begin(uint16_t port,
				const cert_data_type& certPem,
				const cert_data_type& privKeyPem,
				const cert_data_type& caCertPem
		    ) noexcept;

			resBool end() noexcept;

	};

}

namespace https {
	using server = ::http::secure;
}