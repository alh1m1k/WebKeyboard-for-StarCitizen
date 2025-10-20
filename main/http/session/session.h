#pragma once

#include <mutex>

#include "http/session/interfaces/iSession.h"
#include "http/session/interfaces/iSocksCntSession.h"
#include "manager.h"

namespace http::session {

    template <typename T>
    class manager;

    class session: public iSession, public iSocksCntSession {

        protected:

            struct sharedData {
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
                    inline operator bool() {
                        return true;
                    }
                    inline const std::string sid() const {
                        return owner->sid();
                    }
                    virtual sharedData* operator->() {
                        return owner->data();
                    }
            };

            int64_t _updated = -1; //newer
            std::string _sid;
            const uint32_t _index;
            std::shared_mutex _mux;
            sharedData _data = {};
            uint32_t   _traits = 0;
            bool _valid;

            inline virtual sharedData* data() noexcept {
                return &_data;
            }

            inline virtual void invalidate() noexcept {
                _valid = false;
            }

            //warning using this not thread safe, there is no proper locking and int64 not atomic on esp32
            //use ::valid instead, this one for manager thread
            virtual void update(int64_t timestamp) noexcept override;

            session(std::string&& sid, uint32_t index) noexcept;

            void     setSocketCounter(uint32_t value) override;
            uint32_t getSocketCounter()               override;
            uint32_t incSocketCounter(int32_t delta)  override;

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::SESSION;

            using   iSession::features_type;
            typedef sharedData                                     shared_data_type;
            typedef guardian<std::shared_lock<std::shared_mutex>>  read_guardian_type;
            typedef guardian<std::unique_lock<std::shared_mutex>>  write_guardian_type;


            friend class manager<session>;

            virtual ~session() = default;

            inline const std::string& sid() const override {
                return _sid;
            }

            //this once lack of locking at boolean will-be atomic on esp32, unit there is no non trivial op
            inline bool valid() const override {
                return _valid;
            }

            //warning using this not thread safe, there is no proper locking and int64 not atomic on esp32
            //use ::valid instead, this one for manager thread
            virtual bool expired(int64_t timestamp) const noexcept;

            const features_type getTraits() const override;

            virtual inline int32_t duration() const {
                return 60*5;
            }

            virtual read_guardian_type  read() {
                return read_guardian_type(this, std::shared_lock(_mux));
            }

            virtual write_guardian_type write() {
                return write_guardian_type(this, std::unique_lock(_mux));
            }

            inline uint32_t index() const override {
                return _index;
            }

            void * neighbour(uint32_t traitId) override;
            void * downcast() override;

    };

}