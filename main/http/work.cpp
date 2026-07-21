#include "work.h"
#include "util.h"

namespace http {

	void work::staticProcessor(void *arg) {
		const auto node = static_cast<work*>(arg);
		try {
			if (auto returnCode = node->handler(); returnCode != ESP_OK) {
				error("work::staticProcessor error", returnCode);
			}
		} catch (const std::exception& e) {
			error("work::staticProcessor", e.what());
		} catch (...) {
			error("work::staticProcessor", "undefined exception");
		}
		delete node;
	}

	work::work(std::move_only_function<esp_err_t()>&& handler)
		noexcept : handler(std::move(handler)) {}

	work::return_type work::schedule(work::handler_type&& workHandler, httpd_handle_t serverHandle) {
		auto wrk = new work(std::move(workHandler));
		if (const auto code = httpd_queue_work(serverHandle, &work::staticProcessor, wrk); code != ESP_OK) {
			delete wrk;
			return code;
		} else {
			return code;
		}
	}

}

