#pragma once

#include <memory>

#include "http/session/manager.h"
#include "http/session/session.h"
#include "http/session/pointer.h"
#include "http/socket/asyncSocket.h"
#include "http/session/interfaces/iWebSocketSession.h"
#include "sessionManager.h"

using http::session::pointer_cast;

enum class sessionFlags: uint32_t {
    DISCONECTED     = 1 << 0,
	AUTHORIZE= 1 << 1,
    SOCKET_CHANGE   = 1 << 2,
};

struct extendedSessionFields {
    std::string clientName 	= {};
	uint32_t 	heartbeatAtMS = 0;
    uint32_t    commandCnt 	= 0;
    uint32_t    flags      	= 0;
};

class session: public http::session::session<extendedSessionFields>, virtual public http::session::iWebSocketSession {

    friend class sessionManager;
    friend class http::session::manager<session>; //also need to call virt methods

    socket_type _socket = http::socket::noAsyncSocket;

    std::function<void(int type, http::session::iSession* context, void* data)> notification = nullptr;

    protected:
        using http::session::session<extendedSessionFields>::session;
        using http::session::session<extendedSessionFields>::update;

    public:

        static constexpr uint32_t TRAIT_ID = (uint32_t)http::session::traits::TOTAL;

        typedef void(*notification_cb_type)(int type, std::shared_ptr<iSession>& context, void* data);

        using enable_notification_type = std::true_type;

        enum class sessionNotification : int {
            WS_OPEN     = 1000,
            WS_CHANGE,
            WS_CLOSE,
            TOTAL,
        };

        void setWebSocket(const socket_type &socket) override {
            bool isOpen, isClose, isChange = false;
            {
                auto guardian = write();
                if (_socket != socket) {
                    isOpen   = _socket == http::socket::noAsyncSocket;
                    isClose  =  socket == http::socket::noAsyncSocket;
                    guardian->flags |= (uint32_t )sessionFlags::SOCKET_CHANGE;
                    isChange = true;
                    _socket = socket;
                }
            }

            if (isChange && notification != nullptr) {
                if (isOpen) {
                    notification((int)sessionNotification::WS_OPEN,   this, nullptr);
                } else if (isClose) {
                    notification((int)sessionNotification::WS_CLOSE,  this, nullptr);
                } else {
                    notification((int)sessionNotification::WS_CHANGE, this, nullptr);
                }
            }
        }

        bool updateWebSocketIfEq(
            const socket_type& ifEqualToThis, const socket_type& setToThis
        ) override {
            bool isOpen, isClose, isChange = false;
            {
                auto guardian = write();
                if (_socket == ifEqualToThis) {
                    if (_socket != setToThis) {
                        isOpen = _socket == http::socket::noAsyncSocket;
                        isClose = setToThis == http::socket::noAsyncSocket;
                        guardian->flags |= (uint32_t )sessionFlags::SOCKET_CHANGE;
                        isChange = true;
                        _socket = setToThis;
                    }
                }
            }

            if (isChange && notification != nullptr) {
                if (isOpen) {
                    notification((int)sessionNotification::WS_OPEN, this, nullptr);
                } else if (isClose) {
                    notification((int)sessionNotification::WS_CLOSE, this, nullptr);
                } else {
                    notification((int)sessionNotification::WS_CHANGE, this, nullptr);
                }
            }

            return isChange;
        }

        socket_type& getWebSocket() override {
            auto guardian = read();
            return _socket;
        }

        inline void keepAlive(int64_t timestamp) {
            update(timestamp);
        };

        //it is bad idea to override expire to check that socket is authorize and not disconnected
        //as socket may close forced and server close socket handler only if new socked needed
        //atleast until control of wifi phy obtain

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