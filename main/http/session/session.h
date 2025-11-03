#pragma once

#include <mutex>
#include <functional>

#include "task.h"

#include "http/session/interfaces/iSession.h"
#include "http/session/interfaces/iSocksCntSession.h"
#include "manager.h"

namespace http::session {

    template <typename T>
    class manager;

    struct extendedSessionData {};

    template<class TSessionData = extendedSessionData>
    class session: public iSession, virtual public iSocksCntSession {

        portMUX_TYPE _spin = portMUX_INITIALIZER_UNLOCKED;

        protected:

            struct sharedData : public TSessionData {
                size_t pendingSockets = 0;
                size_t index          = 0;
            };

            template<class T>
            class guardian {
                protected:
                    session* const owner;
                    T        lock;
                public:
                    guardian(session* owner, T&& lock) : owner(owner), lock(std::move(lock)) {}
                    explicit inline operator bool() {
                        return true;
                    }
                    inline const std::string sid() const {
                        return owner->sid();
                    }
                    virtual sharedData* operator->() {
                        return owner->data();
                    }
            };

            uint32_t _updatedS = 0;
            std::string _sid;
            std::shared_mutex _mux;
            sharedData _data = {};
            bool _valid;

            inline virtual sharedData* data() noexcept {
                return &_data;
            }

            inline void invalidate(int reason = 0) noexcept override {
                _valid = false;
            }

            //allow to update _updatedS atomically
            //this allow fast non block keepAlive where .expire called on session manager
            void update(int64_t timestamp) noexcept override {
                taskENTER_CRITICAL(&_spin);
                auto _t = (uint32_t)(timestamp / 1000000);
                _updatedS = _t;
                taskEXIT_CRITICAL(&_spin);
            }

            session(std::string&& sid, uint32_t index) noexcept : _sid(sid), _valid(true) {
                _data.index = index;
            }

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::SESSION;

            typedef sharedData                                     shared_data_type;
            typedef guardian<std::shared_lock<std::shared_mutex>>  read_guardian_type;
            typedef guardian<std::unique_lock<std::shared_mutex>>  write_guardian_type;


            friend class manager<session<TSessionData>>;

            virtual ~session() = default;

            [[nodiscard]]
            inline const std::string& sid() const override {
                return _sid;
            }

            //this once lack of locking at boolean will-be atomic on esp32, unit there is no non trivial op
            [[nodiscard]]
            inline bool valid() const override {
                return _valid;
            }

            //warning using this not thread safe, there is no proper locking and int64 not atomic on esp32
            //use ::valid instead, this one for manager thread
            [[nodiscard]]
            bool expired(int64_t timestamp) const noexcept override {
                if (timestamp > 4294967295000000) {
                    error("session impl reach it's limit");
                    esp_restart();
                }
                return (timestamp - (int64_t )_updatedS * 1000000) > ((int64_t )duration() * 1000000);
            }

            [[nodiscard]]
            virtual inline int32_t duration() const {
                return SESSION_TIMEOUT;
            }

            virtual read_guardian_type  read() {
                return read_guardian_type(this, std::shared_lock(_mux));
            }

            virtual write_guardian_type write() {
                return write_guardian_type(this, std::unique_lock(_mux));
            }

            void socksCntInc()  override {
                write()->pendingSockets++;
            }

            void socksCntDecr()  override {
                write()->pendingSockets--;
            }

            [[nodiscard]]
            size_t  socksCnt() const override {
                return _data.pendingSockets;
            }

            [[nodiscard]]
            uint32_t index() const override {
                return _data.index;
            }

            void * neighbour(uint32_t traitId) override {
                switch (traitId) {
                    case iSession::TRAIT_ID:
                        return static_cast<iSession *>(static_cast<session *>(this));
                    case iSocksCntSession::TRAIT_ID:
                        return static_cast<iSocksCntSession *>(static_cast<session *>(this));
                    case TRAIT_ID:
                        return static_cast<session *>(this);
                    default:
                        return nullptr;
                }
            }

            void * downcast() override {
                return static_cast<session*>(this);
            }

    };

}