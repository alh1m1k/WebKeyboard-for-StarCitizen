#pragma once

#include <string>
#include <vector>
#include <span>
#include <algorithm>

#include "http/request.h"
#include "util.h"


namespace crypto::hash {

    template<class rndFnT, class hashEngineT>
    class httpIdentity {

        TRAIT(trait_chunk_hash);
        TRAIT(trait_fill_buffer);

        rndFnT _rnd                     = {};
        std::vector<uint8_t> _secret    = {};

        protected:

            bool salt(const std::span<uint8_t>& buffer, size_t size) {
                if (buffer.size() < size) {
                    return false;
                }
                if constexpr (have_trait_fill_buffer<hashEngineT>::value) {
                    _rnd.fill((void*)buffer.data(), size);
                } else {
                    constexpr int bytes = sizeof(typename rndFnT::result_type);
                    int index = 0;
                    while (size > 0) {
                        auto value = _rnd();
                        for (int bi = 0, offset = 0; bi < bytes && size > 0; ++bi, --size, offset+=8) {
                            buffer[index++] = ((uint8_t)(value >> offset));
                        }
                    }
                }
                return true;
            }

            bool hash(const http::request& req, const std::span<const uint8_t>& _salt, const std::span<uint8_t>& out) const {
                static_assert(have_trait_chunk_hash<hashEngineT>::value, "hashEngineT must provide chunk op");
                #define ensure(eval) if (!(eval)) return false

                hashEngineT engine = {};
                data_type aux(hash_size);

                ensure(engine.begin());
                ensure(engine.hash({ req.getRemote().mac().addr, 6 }, aux));
                engine.chunk(aux);
                engine.chunk(_secret);
                ensure(engine.hash(_salt, aux));
                engine.chunk(aux);
                ensure(engine.end(out));

                return true;
            }

        public:
            static constexpr size_t hash_size        = hashEngineT::hash_size;
            static constexpr size_t salt_size        = std::max(hash_size, (size_t)32);
            static constexpr size_t ident_size       = hash_size + salt_size;

            typedef std::vector<uint8_t> data_type;

            httpIdentity(const std::string& secret) : _secret(hashEngineT().hash({ (uint8_t*)secret.data(), secret.size() })) {
                info("_secret", secret, " ", std::string(_secret.begin(), _secret.end()));
            }

            virtual ~httpIdentity() = default;


            const data_type generate(const http::request& req) {

                data_type out(ident_size);
                salt(out, salt_size);

                if (hash(req, { out.begin(), salt_size }, { out.begin() + salt_size, hash_size })) {
                    return out;
                } else {
                    return hashEngineT::invalidHash;
                }

            }

            bool validate(const http::request& req, const data_type& cid) const {
                if (cid.size() != ident_size) {
                    error("httpIdentity invalid");
                    return false;
                }
                data_type out(hash_size);

                if (hash(req, {cid.begin(), salt_size}, out)) {
                    return std::equal(cid.begin() + salt_size, cid.end(), out.begin());
                } else {
                    return false;
                }

            }
    };

}