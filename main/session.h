#pragma once

#include "http/session/manager.h"
#include "http/session/session.h"
#include "http/session/pointer.h"
#include "http/socket/asyncSocket.h"
#include "http/session/interfaces/iWebSocketSession.h"
#include "sessionManager.h"

using http::session::pointer_cast;

enum class sessionFlags: uint32_t {

};

struct extendedSessionFields {
    std::string clientName = {};
    uint32_t    commandCnt = 0;
    uint32_t    flags      = 0;
};

class session: public http::session::session<extendedSessionFields>, virtual public http::session::iWebSocketSession {

    friend class sessionManager;
    friend class http::session::manager<session>; //also need to call virt methods

    socket_type _socket;
    bool _authorize = false;

    protected:
        using http::session::session<extendedSessionFields>::session;

    public:

        static constexpr uint32_t TRAIT_ID = (uint32_t)http::session::traits::TOTAL;

        inline bool isAuthorized() {
            return _authorize;
        }

        void authorize() {
            _authorize = true;
        }

        void setSocket(const socket_type &socket) override {
            if (_socket != socket) {
                _authorize = false;
                _socket = socket;
            }
        }

        socket_type& getSocket() override {
            return _socket;
        }

        void* neighbour(uint32_t traitId) override {
            switch (traitId) {
                case http::session::iWebSocketSession::TRAIT_ID:
                    return static_cast<http::session::iWebSocketSession*>(static_cast<session*>(this));
                case session::TRAIT_ID:
                    return static_cast<session*>(this);
                default:
                    return http::session::session<extendedSessionFields>::neighbour(traitId);
            }
        }

        void* downcast() override {
            return static_cast<session*>(this);
        }

};