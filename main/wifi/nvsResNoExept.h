#pragma once

#include "generated.h"

#include <memory>
#include <nvs_flash.h>

#include "../exception/bad_api_call.h"
#include "esp_err.h"
#include "../util.h"

namespace wifi {
	
	class nvsResNoExept {

	static inline std::weak_ptr<nvsResNoExept> storage;
	
	esp_err_t code;

	public: 
	
		//todo better use DI container for this
		static std::shared_ptr<nvsResNoExept> acquire() noexcept {
			if (auto shared = storage.lock(); shared) {
				return shared;
			} else {
				shared = std::make_shared<nvsResNoExept>();
				storage = shared;
				return shared;
			}
		}
		
		nvsResNoExept() noexcept {
			if (code = nvs_flash_init(); code != ESP_OK) {
				debugIf(LOG_WIFI, "nvs_init fail", code);
			} else {
				debugIf(LOG_WIFI, "nvs_init");
			}
		}
		
		virtual ~nvsResNoExept() noexcept {
			if (code = nvs_flash_deinit(); code != ESP_OK) {
				debugIf(LOG_WIFI, "nvs_deinit fail", code);
			} else {
				debugIf(LOG_WIFI, "nvs_deinit");
			}
		
		}
		
		inline bool valid() const noexcept {
			return code == ESP_OK;
		}
		
		operator bool() const noexcept {
			return valid();
		}
	};	

}