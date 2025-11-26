#pragma once

#include <span>


#include "result.h"

namespace compression {

    class gzEngine {
        public:
            typedef std::span<const uint8_t>    ptr_type_in;
            typedef std::span<uint8_t>          ptr_type_out;

            const static auto INPUT_EXHAUSTED  = 1;
            const static auto OUTPUT_FULL      = 2;

            gzEngine() noexcept;

            typedef std::true_type trait_decompress;
            resBool decompress(const ptr_type_in& input, size_t& read, const ptr_type_out& output, size_t& writen) noexcept;
            

            typedef std::false_type trait_compress; //not needed
    };

}
