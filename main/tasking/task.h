#pragma once

#include "generated.h"

#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"

#include "util.h"
#include "bad_api_call.h"

namespace tasking {

	class task {

		TaskHandle_t handle = nullptr;

		explicit task() = default;

		public:

			static const task terminated;

			static inline void this_sleep_for(const uint32_t delayMs) noexcept {
				vTaskDelay(pdMS_TO_TICKS(delayMs));
			}

			static inline void this_yield() noexcept {
				taskYIELD();
			}

			explicit task(
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

			explicit task(
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

			virtual ~task() noexcept {
				if (handle != nullptr) {
					vTaskDelete(handle);
					handle = nullptr;
				}
			};

			task(task&) = delete;

			task& operator=(task&) = delete;

			inline bool operator==(const task& other) const noexcept {
				return handle == other.handle;
			}

			[[nodiscard]] inline TaskHandle_t native() const noexcept {
				return handle;
			}

			inline void notify(const uint32_t ulValue, const eNotifyAction eAction) noexcept {
				xTaskNotify(handle, ulValue, eAction);
			}

	};

}


