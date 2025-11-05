#pragma once

#include "session.h"
#include "request.h"
#include "http/session/manager.h"
#include "crypto/hash/httpIdentity.h"
#include "crypto/hash/sha256Engine.h"

class sessionManager: public http::session::manager<class session> {

    typedef  http::session::manager<class session> base;
    typedef  crypto::hash::httpIdentity<hwrandom<int>, crypto::hash::sha256Engine> sid_generator_type;

    protected:

        sid_generator_type identity;

        const std::function<void(int type, http::session::iSession* context, void* data)> memberNotification = nullptr;

        std::string generateSID(const http::request *context = nullptr) override;

        bool validateSession(std::shared_ptr<http::session::iSession> &session, const http::request *context = nullptr) const override;

        void processMemberNotification(int type, http::session::iSession* context, void* data) const;

        manager<session>::session_type* makeSession(const http::request *context = nullptr) override;

        void updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const override;

        void invalidateSessionPtr(session_ptr_type& sessionPtr, int reason = 0) const override;

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

        sessionManager(const std::string& secret) :
            identity(secret),
            memberNotification([manager = this](int type, http::session::iSession* context, void* data) -> void {
                manager->processMemberNotification(type, context, data);
            })
        {

        };

        virtual ~sessionManager() = default;

        std::function<void(int type, session_ptr_type& context, void* data)> notification = nullptr;

        result_type open(const http::request *context = nullptr) override;

        result_type open(const std::string &sid, const http::request *context = nullptr) override;

        void* neighbour(uint32_t traitId) override;

        void* downcast() override;
};