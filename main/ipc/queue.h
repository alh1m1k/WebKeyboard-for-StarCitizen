#pragma once

#include "generated.h"

#include <type_traits>
#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "util.h"
#include "bad_api_call.h"

namespace ipc {

	template <typename ITEM>
	class queue {

		static_assert(std::is_trivially_copyable<ITEM>::value, "ITEM must be memcpy compatible");

		QueueHandle_t const handle = nullptr; // NOLINT(misc-misplaced-const)

	public:

		explicit queue(const UBaseType_t size): handle(xQueueCreate(size, sizeof(ITEM))) {
			if (handle == nullptr) {
				throw bad_api_call("queue::queue", ESP_FAIL);
			}
		};

		~queue() noexcept {
			vQueueDelete(handle);
		};

		queue(queue&) = delete;

		queue& operator=(queue&) = delete;

		inline QueueHandle_t native() const noexcept {
			return handle;
		}

		resBool push_back(const ITEM& value, const TickType_t waitTicks) noexcept {
			return xQueueSendToBack(handle, &value, waitTicks) == pdPASS ? ResBoolOK : ResBoolFAIL;
		}

		result<ITEM> pop_front(const TickType_t waitTicks) noexcept {
			ITEM value;
			if (auto result = xQueueReceive(handle, &value, waitTicks); result == pdPASS) {
				return std::move(value);
			} else {
				return ESP_FAIL; //todo change me
			}
		}

		bool pop_front(ITEM& outValue, const TickType_t waitTicks) noexcept {
			return xQueueReceive(handle, &outValue, waitTicks) == pdPASS;
		}

		[[nodiscard]] size_t size() const noexcept {
			return uxQueueMessagesWaiting(handle);
		}

		[[nodiscard]] bool empty() const noexcept {
			return size() == 0;
		}

		[[nodiscard]] size_t available() const noexcept {
			return uxQueueSpacesAvailable(handle);
		}

		[[nodiscard]] bool full() const noexcept {
			return available() == 0;
		}
	};

}


