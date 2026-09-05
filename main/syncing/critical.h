#pragma once


#include "freertos/FreeRTOS.h"
#include "portmacro.h"

namespace syncing {

    class critical {
        portMUX_TYPE spin = portMUX_INITIALIZER_UNLOCKED;
        public:

    		using native_handle_type = portMUX_TYPE;

            inline void lock() noexcept {
                portENTER_CRITICAL(&spin);
            };

    		inline bool try_lock() noexcept {
    			return portTRY_ENTER_CRITICAL(&spin, 0) == pdTRUE;
    		};

            inline void unlock() noexcept {
                portEXIT_CRITICAL(&spin);
            };

    		[[nodiscard]] native_handle_type native_handle() const noexcept {
    			return spin;
    		}
    };

}