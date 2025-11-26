#include "gzEngine.h"

#include <stdexcept>

namespace compression {

    gzEngine::gzEngine() noexcept {
    }

    resBool gzEngine::decompress(const ptr_type_in& input, size_t& read, const ptr_type_out& output, size_t& writen) noexcept {
        return ESP_OK;
    }

}