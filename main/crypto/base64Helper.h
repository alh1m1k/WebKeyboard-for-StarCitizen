#pragma once

#include <string>
#include <vector>
#include <span>

#include <mbedtls/base64.h>

namespace crypto {

    class base64Helper {
        public:
            static std::string toBase64(const std::span<const uint8_t>& rawIn) {
                size_t b64_buffer_size = ((rawIn.size() + 2) / 3) * 4 + 1;
                std::string outBuffer = {};
                outBuffer.resize(b64_buffer_size);
                size_t outSize = 0;

                //((rawIn.size() + 2) / 3) * 4 + 1 why size is +1
                //when we use buffer with magnitude of 4 ie 88 mbedtls_base64_encode fail with non 0 error code
                //require +1 additional byte, but when we use buffer 89 it return size magnitude of 4 ie 88
                if (mbedtls_base64_encode((unsigned char*)outBuffer.data(), outBuffer.size(), &outSize, (const unsigned char*)rawIn.data(), rawIn.size()) != 0) {
                    outBuffer.resize(0);
                } else {
                    outBuffer.resize(outSize);
                }

                return outBuffer;
            }

            static std::vector<uint8_t> fromBase64(const std::string& base64in) {
                size_t raw_buffer_size = (((base64in.size()) + 3) / 4) * 3;

                std::vector<uint8_t> outBuffer(raw_buffer_size);
                size_t outSize = 0;

                if (mbedtls_base64_decode((unsigned char*)outBuffer.data(), outBuffer.size(), &outSize, (const unsigned char*)base64in.data(), base64in.size()) != 0) {
                    outBuffer.resize(0);
                } else {
                    outBuffer.resize(outSize);
                }

                return outBuffer;
            }
    };

}
