#pragma once

#include <mutex>
#include <atomic>
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

        protected:

            struct sharedData : public TSessionData {
                size_t pendingSockets = 0;
                size_t index          = 0;
            };

            template<class LockT, bool ConstType>
            class guardian {
                protected:
                    session* const owner;
                    LockT          lock;
                public:
                    guardian(session* owner, LockT&& lock) : owner(owner), lock(std::move(lock)) {}
                    explicit inline operator bool() const {
                        return true;
                    }
                    inline const std::string sid()  const {
                        return owner->sid();
                    }
                    virtual std::conditional<ConstType, const sharedData, sharedData>::type* operator->() const {
                        return owner->data();
                    }
            };

            std::atomic<uint32_t> _updatedExpiredS   = 0;
            std::atomic<uint32_t> _updatedOutdatedS  = 0;
            std::string _sid;
            std::shared_mutex _mux;
            sharedData _data = {};
            bool _valid;

            inline virtual sharedData* data() noexcept {
                return &_data;
            }

            //must be called only by manager
            inline void invalidate(int reason = 0) noexcept override {
                _valid = false;
            }

            //must be called only by manager
            void update(int64_t timestamp) noexcept override {
                auto _t = (uint32_t)(timestamp / 1000000);
                _updatedExpiredS = _t;
            }

            //must be called only by manager
            void renew(const sid_type& sid, int64_t timestamp) noexcept override {
                auto _t = (uint32_t)(timestamp / 1000000);
                _sid = sid;
                _updatedOutdatedS = _t;
            }

            session(std::string&& sid, uint32_t index, int64_t timestamp) noexcept : _sid(sid), _valid(true) {
                _data.index = index;
                _updatedOutdatedS = _updatedExpiredS = (uint32_t)(timestamp / 1000000);
            }

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::SESSION;

            typedef sharedData                                     shared_data_type;
            typedef guardian<std::shared_lock<std::shared_mutex>, true>   read_guardian_type;
            typedef guardian<std::unique_lock<std::shared_mutex>, false>  write_guardian_type;


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

            //not safe to call outside manager
            [[nodiscard]]
            bool expired(int64_t timestamp) const noexcept override {
				assert(timestamp <= 4294967295000000);
                auto timeMark = _updatedExpiredS.load();
                return (timestamp - (int64_t )timeMark * 1000000) > ((int64_t )duration() * 1000000);
            }

            [[nodiscard]]
            bool outdated(int64_t timestamp) const noexcept override {
				assert(timestamp <= 4294967295000000);
                auto timeMark = _updatedOutdatedS.load();
                return (timestamp - (int64_t )timeMark * 1000000) > ((int64_t )refreshInterval() * 1000000);
            }

            [[nodiscard]]
            virtual inline int32_t duration() const {
                static_assert(SESSION_SID_REFRESH_INTERVAL > SESSION_TIMEOUT, "SESSION_SID_REFRESH_INTERVAL > SESSION_TIMEOUT");
                return SESSION_TIMEOUT;
            }

            [[nodiscard]]
            virtual inline int32_t refreshInterval() const {
                static_assert(SESSION_SID_REFRESH_INTERVAL > SESSION_TIMEOUT, "SESSION_SID_REFRESH_INTERVAL > SESSION_TIMEOUT");
                return SESSION_SID_REFRESH_INTERVAL;
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