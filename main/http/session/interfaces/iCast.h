#pragma once

#include <cstdint>

#include "features.h"

namespace http::session {

    class iCast {
        public:
            virtual void* neighbour(uint32_t traitId) = 0;
            virtual void* downcast() = 0;
    };

}
