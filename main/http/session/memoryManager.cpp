#include "memoryManager.h"

namespace http::session {

    result<baseSession*> memoryManager::open() {
        return nullptr;
    }

    result<baseSession*> memoryManager::open(const std::string_view id) {
        return nullptr;
    }

    resBool close(const std::string_view id) {
        return true;
    }

}