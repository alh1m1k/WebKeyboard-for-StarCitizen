#pragma once

#include <span>

#include "result.h"

namespace compression {

    template<class TEngine>
    class decompressor {
        TEngine engine = {};

        class walker {
        decompressor<TEngine>&       owner;
        const TEngine::ptr_type_in&  input;
        size_t read   = 0;
        size_t writen = 0;
        public:
            walker(decompressor<TEngine>& owner, const TEngine::ptr_type_in& input) : owner(owner), input(input) {}
            resBool next(const TEngine::ptr_type_out& output, size_t& w) {
                if (read >= input.size()) {
                    return ESP_FAIL;
                }
                size_t r = 0;
                if (auto result = owner.decompress(input.subspan(read), r, output, w); result.code() >= 0) {
                    read    += r;
                    writen  += w;
                    return result.code() > 0;
                } else {
                    return result;
                }
            }
/*            result<const typename TEngine::ptr_type_out> next(const TEngine::ptr_type_out& output) {
                size_t r, w = 0;
                if (auto result = owner.decompress(input.subspan(read), r, output, w); result.code() >= 0) {
                    read    += r;
                    writen  += w;
                    return output.subspan(0, w);
                } else {
                    return result;
                }
            }*/
            inline size_t total() {
                return writen;
            }
        };

        public:

            typedef TEngine::ptr_type_in  ptr_type_in;
            typedef TEngine::ptr_type_out ptr_type_out;


            inline resBool decompress(const ptr_type_in& input, size_t& read, const ptr_type_out& output, size_t& writen) {
                return engine.decompress(input, read, output, writen);
            };

            walker decompress(const ptr_type_in& input) {
                return walker(*this, input);
            };
    };

}