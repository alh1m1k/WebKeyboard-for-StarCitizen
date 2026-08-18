#pragma once

#include "generated.h"

#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"

#include "util.h"
#include "bad_api_call.h"

namespace task {

	class generic {

		TaskHandle_t handle = nullptr;

		explicit generic() = default;

		public:

			static const generic terminated;

			explicit generic(
				TaskFunction_t pxTaskCode,
				const char * const pcName,
				const size_t usStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority
			) {
				if (const auto code = xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &handle); code != pdPASS) {
					throw bad_api_call("xTaskCreate fail to create task", code);
				}
			};

			explicit generic(
				TaskFunction_t pxTaskCode,
				const char * const pcName,
				const size_t usStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority,
				const BaseType_t xCoreID
			) {
				if (auto code = xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &handle, xCoreID); code != pdPASS) {
					throw bad_api_call("xTaskCreate fail to create task", code);
				}
			};

			virtual ~generic() noexcept {
				if (handle != nullptr) {
					vTaskDelete(handle);
					handle = nullptr;
				}
			};

			generic(generic&) = delete;

			generic& operator=(generic&) = delete;

			inline bool operator==(const generic& other) const noexcept {
				return handle == other.handle;
			}

			[[nodiscard]] inline TaskHandle_t native() const noexcept {
				return handle;
			}

	};

}


