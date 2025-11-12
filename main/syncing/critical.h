#pragma once

#include "freertos/task.h"

namespace syncing {

    class critical {
        portMUX_TYPE spin = portMUX_INITIALIZER_UNLOCKED;
        public:
            inline void lock() {
                taskENTER_CRITICAL(&spin);
            };
            inline void unlock() {
                taskEXIT_CRITICAL(&spin);
            };
    };

}