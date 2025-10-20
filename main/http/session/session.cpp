#include "session.h"

#include "generated.h"

#include "util.h"

namespace http::session {

    session::session(std::string&& sid, uint32_t index) noexcept  :
            iSession(_traits),
            iSocksCntSession(_traits),
            _sid(sid),
            _index(index),
            _valid(true)
    {

    };

    void session::update(int64_t timestamp) noexcept {
        _updated = timestamp;
    }

    const session::features_type session::getTraits() const {
        return features(_traits);
    }

    bool session::expired(int64_t timestamp) const noexcept {
        if (_updated == -1) {
            return false;
        }
        return (timestamp - _updated) > ((int64_t )duration() * 1000000);
    }

    void session::setSocketCounter(uint32_t value) {
        _data.pendingSockets = value;
    }

    uint32_t session::getSocketCounter() {
        return _data.pendingSockets;
    }

    uint32_t session::incSocketCounter(int32_t delta) {
        return write()->pendingSockets += delta;
    }

    void* session::neighbour(uint32_t traitId) {
        switch (traitId) {
            case iSession::TRAIT_ID:
                return static_cast<iSession*>(static_cast<session*>(this));
            case iSocksCntSession::TRAIT_ID:
                return static_cast<iSocksCntSession*>(static_cast<session*>(this));
            case TRAIT_ID:
                return static_cast<session*>(this);
            default:
                return nullptr;
        }
    }

    void* session::downcast() {
        return static_cast<session*>(this);
    }

}