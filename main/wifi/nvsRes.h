#pragma once

#include "generated.h"

#include <memory>
#include <nvs_flash.h>

#include "../exception/bad_api_call.h"
#include "esp_err.h"
#include "../util.h"

namespace wifi {
	
	class nvsRes {

	static inline std::weak_ptr<nvsRes> storage;

	public: 
	
		//todo better use DI container for this
		static std::shared_ptr<nvsRes> acquire() {
			if (auto shared = storage.lock(); shared) {
				return shared;
			} else {
				shared = std::make_shared<nvsRes>();
				storage = shared;
				return shared;
			}
		}
		
		nvsRes() {
			if (auto code = nvs_flash_init(); code != ESP_OK) {
				throw bad_api_call("nvsRes init", code);
			} else {
				debugIf(LOG_WIFI, "nvs_init");
			}
		}
		virtual ~nvsRes() {
			if (auto code = nvs_flash_deinit(); code != ESP_OK) {
				error("nvsRes:~nvsRes nvs_deinit", code);
			} else {
				debugIf(LOG_WIFI, "nvs_deinit");
			}
		
		}
	};	

}
