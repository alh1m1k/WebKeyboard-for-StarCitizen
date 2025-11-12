#include "sessionManager.h"
#include "crypto/base64Helper.h"
#include "syncing/dummy.h"

std::string sessionManager::generateSID(const http::request *context) {
    if (context == nullptr) {
        throw std::logic_error("context must be provided");
    }
    if (auto sidBuffer = identity.generate(*context); sidBuffer != sid_generator_type::engine_type::invalidHash) {
        return crypto::base64Helper::toBase64(sidBuffer);
    } else {
        throw std::logic_error("hash op error");
    }
}

bool sessionManager::validateSession(std::shared_ptr<http::session::iSession> &session, const http::request *context) const {
    if (context == nullptr) {
        throw std::logic_error("context must be provided");
    }
    if (auto sidBuffer = crypto::base64Helper::fromBase64(session->sid()); sidBuffer.size() == sid_generator_type::ident_size) {
        return identity.validate(*context, sidBuffer);
    } else {
        error("invalid ident size");
        return false;
    }
}

void sessionManager::processMemberNotification(int type, http::session::iSession* context, void* data) const {
    if (auto sess = find([context](const session_ptr_type& sess) -> bool {
            return sess.get() == context;
        }, std::shared_lock<syncing::dummy>()); sess != nullptr) {
        notification(type, sess, data);
    } else {
        error("session that emit event does not exist", context->sid().c_str());
    }
}

sessionManager::session_type* sessionManager::makeSession(const http::request *context, int64_t timestamp)  {
    auto sess = new session(generateSID(context), index(), timestamp ? timestamp : esp_timer_get_time());
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
