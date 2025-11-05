#include "sha256Engine.h"

namespace crypto::hash {

    sha256Engine::~sha256Engine() {
        if (_ctx != nullptr) {
            mbedtls_sha256_free(_ctx.get());
        }
    }

    bool sha256Engine::hash(const ptr_type_in& in, const ptr_type_out& out) {
        if (out.size() >= hash_size) {
            if (error = mbedtls_sha256((unsigned char*)in.data(), in.size(), (unsigned char*)out.data(), 0); error == 0) {
                return true;
            }
        }
        return false;
    }

    const sha256Engine::data_type sha256Engine::hash(const ptr_type_in& in) {
        data_type out(hash_size);
        if (hash(in, out)) {
            return out;
        } else {
            return invalidHash;
        }
    }

    bool sha256Engine::begin() {
        _ctx = std::make_unique<mbedtls_sha256_context>();
        mbedtls_sha256_init(_ctx.get());
        if (error = mbedtls_sha256_starts(_ctx.get(), 0); error != 0) {
            mbedtls_sha256_free(_ctx.get());
            return false;
        }
        return true;
    }

    bool sha256Engine::chunk(const ptr_type_in& in) {
        return mbedtls_sha256_update(_ctx.get(), in.data(), in.size()) == 0;
    }

    bool sha256Engine::end(const ptr_type_out& out) {
        error = mbedtls_sha256_finish(_ctx.get(), out.data());
        mbedtls_sha256_free(_ctx.get());
        return error == 0;
    }

    sha256Engine::data_type sha256Engine::end() {
        data_type out(hash_size);
        if (end(out)) {
            return out;
        } else {
            return invalidHash;
        }
    }

}

