#pragma once

#include "session.h"
#include "request.h"
#include "http/session/manager.h"

class sessionManager: public http::session::manager<class session> {

    typedef  http::session::manager<class session> base;

    protected:

        const std::function<void(int type, http::session::iSession* context, void* data)> memberNotification = nullptr;

        void processMemberNotification(int type, http::session::iSession* context, void* data) const {
            if (auto sess = find(context->sid()); sess != nullptr) {
                notification(type, sess, data);
            } else {
                error("session that emit event does not exist", context->sid().c_str());
            }
        }

        manager<session>::session_type* makeSession(const http::request *context = nullptr) override {
            auto sess = new session(generateSID(context), index());
            if (notification != nullptr) {
                sess->notification = memberNotification;
            }
            return sess;
        }

        void updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const override {
            std::static_pointer_cast<session>(sessionPtr)->update(timestamp);
        }

        void invalidateSessionPtr(session_ptr_type& sessionPtr, int reason = 0) const override {
            std::static_pointer_cast<session>(sessionPtr)->invalidate();
            if (notification != nullptr) {
                notification((int)sessionNotification::CLOSE, sessionPtr, (void*)&reason);
            }
        }

    public:

        static constexpr uint32_t TRAIT_ID = (uint32_t)session::TRAIT_ID + 1;

        typedef void(*notification_cb_type)(int type, session_ptr_type& context, void* data);
        typedef struct { uint32_t reason; const http::request* req; } note_open_type;
        using enable_notification_type = std::true_type;

        enum class sessionNotification : int {
            OPEN = 1,
            CLOSE,
            TOTAL,
        };

        sessionManager(): memberNotification([manager = this](int type, http::session::iSession* context, void* data) -> void {
            manager->processMemberNotification(type, context, data);
        }) {

        };

        virtual ~sessionManager() = default;

        std::function<void(int type, session_ptr_type& context, void* data)> notification = nullptr;


        result_type open(const http::request *context = nullptr) override {
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

        result_type open(const std::string &sid, const http::request *context = nullptr) override {
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

        void* neighbour(uint32_t traitId) override {
            switch (traitId) {
                case sessionManager::TRAIT_ID:
                    return static_cast<sessionManager*>(this);
                default:
                    return http::session::manager<session>::neighbour(traitId);
            }
        }

        void* downcast() override {
            return static_cast<sessionManager*>(this);
        }
};