#include "sessionManager.h"

#include <mbedtls/base64.h>

std::string sessionManager::generateSID(const http::request *context) {
    if (context == nullptr) {
        throw std::logic_error("context must be provided");
    }
    constexpr size_t b64_buffer_size = ((sid_generator_type::ident_size + 2) / 3) * 4 + 1; //+1 for '=' at end;

    auto inBuffer = identity.generate(*context);
    std::string outBuffer = {};
    outBuffer.resize(b64_buffer_size);
    size_t outSize = 0;

    if (mbedtls_base64_encode((unsigned char*)outBuffer.data(), outBuffer.size(), &outSize, (const unsigned char*)inBuffer.data(), inBuffer.size()) != 0) {
        outBuffer.resize(0);
    } else {
        outBuffer.resize(outSize);
    }

    return outBuffer;
}

bool sessionManager::validateSession(std::shared_ptr<http::session::iSession> &session, const http::request *context) const {
    if (context == nullptr) {
        throw std::logic_error("context must be provided");
    }

    auto inBuffer = session->sid();
    std::vector<uint8_t> outBuffer(sid_generator_type::ident_size);
    size_t outSize = 0;

    if (mbedtls_base64_decode((unsigned char*)outBuffer.data(), outBuffer.size(), &outSize, (const unsigned char*)inBuffer.data(), inBuffer.size()) != 0) {
        return false;
    } else {
        if (outSize != sid_generator_type::ident_size) {
            error("invalid ident size");
            return false;
        }
        outBuffer.resize(outSize);
    }

    return identity.validate(*context, outBuffer);
}

void sessionManager::processMemberNotification(int type, http::session::iSession* context, void* data) const {
    if (auto sess = find(context->sid()); sess != nullptr) {
        notification(type, sess, data);
    } else {
        error("session that emit event does not exist", context->sid().c_str());
    }
}

sessionManager::session_type* sessionManager::makeSession(const http::request *context)  {
    auto sess = new session(generateSID(context), index());
    if (notification != nullptr) {
        sess->notification = memberNotification;
    }
    return sess;
}

void sessionManager::updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const  {
    std::static_pointer_cast<session>(sessionPtr)->update(timestamp);
}

void sessionManager::invalidateSessionPtr(session_ptr_type& sessionPtr, int reason) const  {
    std::static_pointer_cast<session>(sessionPtr)->invalidate();
    if (notification != nullptr) {
        notification((int)sessionNotification::CLOSE, sessionPtr, (void*)&reason);
    }
}

sessionManager::result_type sessionManager::open(const http::request *context)  {
    if (auto r = base::open(context); r) {
        if (notification != nullptr) {
            note_open_type details = {
                .reason  = 1,
                .req    = context,
            };
            notification((int)sessionNotification::OPEN, std::get<session_ptr_type>(r), (void*)&details);
        }
        return r;
    } else {
        return r;
    }
}

sessionManager::result_type sessionManager::open(const std::string &sid, const http::request *context)  {
    if (auto r = base::open(sid, context); r) {
        if (notification != nullptr) {
            note_open_type details = {
                .reason  = 2,
                .req    = context,
            };
            notification((int)sessionNotification::OPEN, std::get<session_ptr_type>(r), (void*)&details);
        } else {
            error("no manager");
        }
        return r;
    } else {
        return r;
    }
}

void* sessionManager::neighbour(uint32_t traitId)  {
    switch (traitId) {
        case sessionManager::TRAIT_ID:
            return static_cast<sessionManager*>(this);
        default:
            return http::session::manager<session>::neighbour(traitId);
    }
}

void* sessionManager::downcast()  {
    return static_cast<sessionManager*>(this);
}
