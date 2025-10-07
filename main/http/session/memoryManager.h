#pragma once

#include "baseManager.h"

namespace http::session {

    class memoryManager: public baseManager {
        public:
            result<baseSession*> open() override;
            result<baseSession*> open(const std::string_view id) override;
            resBool close(const std::string_view id) override;
    };

}
