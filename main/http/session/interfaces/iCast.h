#pragma once

#include <cstdint>

#include "features.h"

namespace http::session {

    class iCast {
        public:
            typedef features<uint32_t > features_type;

            virtual const features_type getTraits() const = 0;
            virtual void* neighbour(uint32_t traitId) = 0;
            virtual void* downcast() = 0;
    };

}
