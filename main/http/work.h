#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "generated.h"
#include "result.h"
#include <functional>

namespace http {
	class work {

		static void staticProcessor(void *arg);

		std::move_only_function<esp_err_t()> handler;

		explicit work(std::move_only_function<esp_err_t()>&& handler) noexcept;

		public:

		typedef std::move_only_function<esp_err_t()> handler_type;
		typedef resBool return_type;

		static return_type schedule(std::move_only_function<esp_err_t()>&& workHandler, httpd_handle_t serverHandle);

	};
}