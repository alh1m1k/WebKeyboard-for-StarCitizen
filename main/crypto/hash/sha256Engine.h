#pragma once

#include <memory>
#include <vector>
#include <span>

#include "mbedtls/sha256.h"

#include "esp_err.h"

namespace crypto::hash {

    class sha256Engine {

        std::unique_ptr<mbedtls_sha256_context> _ctx = nullptr;

        public:
            typedef std::vector<uint8_t>        data_type;
            typedef std::span<const uint8_t>    ptr_type_in;
            typedef std::span<uint8_t>          ptr_type_out;

            static constexpr size_t hash_size = 256/8;

            static constexpr data_type invalidHash = {};

            sha256Engine()          = default;
            virtual ~sha256Engine();

            int error = 0; //change it to result<std::vector<uint8_t>> aster resolve all question about copy/move

            using trait_inplace_hash = std::true_type;
            const data_type hash(const ptr_type_in& in);
                  bool      hash(const ptr_type_in& in, const ptr_type_out& out);

            using trait_chunk_hash   = std::true_type;
            bool begin();
            bool chunk(const ptr_type_in&  chunk);
            bool end(  const ptr_type_out& out );
            data_type end();
    };

}


