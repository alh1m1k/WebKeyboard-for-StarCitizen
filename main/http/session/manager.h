#pragma once

#include "generated.h"

#include <shared_mutex>
#include <array>
#include <type_traits>

#include "esp_timer.h"

#include "http/session/interfaces/iManager.h"
#include "http/session/session.h"
#include "util.h"
#include "hwrandom.h"

namespace http::session {

#define SESSION_ERROR_BASE      10000
#define NO_SLOT_SESSION_ERROR   (SESSION_ERROR_BASE + 1)
#define COLLISION_SESSION_ERROR (SESSION_ERROR_BASE + 2)
#define INVALID_SESSION_ERROR   (SESSION_ERROR_BASE + 3)


    template<class TSession>
    class manager: public iManager {

        struct {
            mutable std::shared_mutex   rwMutex;
            std::array<std::shared_ptr<iSession>, SESSION_MAX_CLIENT_COUNT> data = {nullptr};
            size_t  count = 0;
        } storage;

        generator<uint32_t, false> _generator = {0};

        result<session_ptr_type> placeNew(std::shared_ptr<iSession>& session, int64_t timestamp) {

            std::shared_ptr<iSession>* slot = nullptr;
            std::shared_ptr<iSession> expireSession;
            for (size_t i = 0; i < storage.data.size(); ++i) {
                if (storage.data[i] != nullptr) {
                    if (!storage.data[i]->expired(timestamp)) {
                        if (storage.data[i]->sid() == session->sid()) {
                            return COLLISION_SESSION_ERROR;
                        }
                    } else if (slot == nullptr) {
                        slot            = &storage.data[i];
                        expireSession   = storage.data[i]; //better don't move from array, even if it valid
                    }
                } else if (slot == nullptr) {
                    slot = &storage.data[i];
                }
            }

            if (slot == nullptr) {
                return NO_SLOT_SESSION_ERROR;
            } else {
                if (expireSession == nullptr) {
                    storage.count++;
                }
                *slot = session;
            }

            if (expireSession != nullptr) {
                return expireSession;
            }

            return ESP_OK;
        }

        session_ptr_type findBy(const sid_type& sid) const {

            if (storage.count == 0) {
                return nullptr;
            }

            for (size_t i = 0; i < total; ++i) {
                if (storage.data[i] != nullptr) {
                    if (storage.data[i]->sid() == sid) {
                        return storage.data[i];
                    }
                }
            }

            return nullptr;
        }

        inline session_ptr_type findBy(const index_type& index) const {
            return findBy([&index](const session_ptr_type& sess) -> bool  {
                return sess->index() == index;
            });
        }

        session_ptr_type findBy(const std::function<bool(const session_ptr_type& sess)>& callback) const {

            if (storage.count == 0) {
                return nullptr;
            }

            for (size_t i = 0; i < total; ++i) {
                if (storage.data[i] != nullptr) {
                    if (callback(storage.data[i])) {
                        return storage.data[i];
                    }
                }
            }

            return nullptr;
        }

        inline session_ptr_type removeBy(const sid_type& sid) {
            return removeBy([&sid](const session_ptr_type& sess) -> bool  {
                return sess->sid() == sid;
            });
        }

        session_ptr_type removeBy(const session_ptr_type& sess) {

            if (storage.count == 0) {
                return nullptr;
            }

            for (size_t i = 0; i < total; ++i) {
                if (storage.data[i] != nullptr) {
                    if (storage.data[i] == sess) {
                        storage.data[i] = nullptr;
                        return sess;
                    }
                }
            }

            return nullptr;
        }

        session_ptr_type removeBy(const std::function<bool(const session_ptr_type& sess)>& callback) {

            if (storage.count == 0) {
                return nullptr;
            }

            for (size_t i = 0; i < total; ++i) {
                if (storage.data[i] != nullptr) {
                    if (callback(storage.data[i])) {
                        auto& prev = storage.data[i];
                        storage.data[i] = nullptr;
                        return prev;
                    }
                }
            }

            return nullptr;
        }

        protected:

        typedef std::shared_lock<std::shared_mutex> ro_lock_type;
        typedef std::unique_lock<std::shared_mutex> rw_lock_type;

        template<bool constType>
        class manager_iterator {
            typedef std::array<std::shared_ptr<iSession>, SESSION_MAX_CLIENT_COUNT> container_type;
            typedef std::conditional<constType, container_type::const_iterator, container_type::iterator>::type  internal_it_type;

            internal_it_type    it;
            internal_it_type    begin;
            internal_it_type    end;
            int64_t             timestamp;

            public:
                using difference_type 	= container_type::difference_type;
                using value_type 		= container_type::value_type;
                using pointer 			= std::conditional<constType, container_type::const_pointer, container_type::pointer>::type;
                using reference 		= std::conditional<constType, container_type::const_reference, container_type::reference>::type;
                using iterator_category = std::bidirectional_iterator_tag;

                explicit manager_iterator(internal_it_type begin, internal_it_type end, int64_t timestamp) : it(begin), begin(begin), end(end), timestamp(timestamp) {
                    //c++ it half opened to we can iterate safly from begin to end
                    //this ugly code can be plased there on inside .begin
                    if (it != end) { //it is not ended
                        if ((*it) == nullptr || (*it)->expired(timestamp)) { //current pos was invalid
                            this->operator++(); //find next valid pos
                        }
                    }
                };
                manager_iterator(const manager_iterator& copy)                  = default;
                manager_iterator(manager_iterator &&move)                       = default;
                    manager_iterator& operator=(const manager_iterator& copy)   = default;
                    value_type& operator*() const {
                        return *it;
                    }
                    auto operator++() {
                        for (++it; it != end; ++it) {
                            if (*it != nullptr && !(*it)->expired(timestamp)) {
                                break;
                            }
                        }
                    }
                    auto operator--() {
                        //begin() - 1 is invalid, so it is irrelevant valid it or not
                        for (--it; it != begin; --it) {
                            if (*it != nullptr && !(*it)->expired(timestamp)) {
                                return;
                            }
                        }
                    }
                    bool operator==(const manager_iterator& other) const {
                        return other.it == this->it;
                    }
                    bool operator!=(const manager_iterator& other) const {
                        return other.it != this->it;
                    }
                    bool operator>(const manager_iterator& other) const {
                        return other.it > this->it;
                    }
            };

            template<class LockT>
            inline LockT guard() const {
                return LockT(storage.rwMutex);
            }

            template<class LockT>
            inline result<std::shared_ptr<iSession>> place(std::shared_ptr<iSession>& session, int64_t timestamp, [[maybe_unused]] LockT&& _g) {
                if constexpr (std::is_rvalue_reference<LockT&&>::value) {
                    LockT guardian = std::move(_g);
                    return placeNew(session, timestamp);
                } else {
                    return placeNew(session, timestamp);
                }
            }

            template<class indexT, class LockT>
            inline std::shared_ptr<iSession> find(const indexT& index, [[maybe_unused]] LockT&& _g) const {
                //this concept on opt locking api for private storage
                //if guardian is moved than find is owning lock (take ownership) and release it after function ending
                //if guardian is referenced lock is no owning, ie no lock released by find
                //if LockT is ro_lock_type it is lock in share mode
                //if LockT is rw_lock_type it's unique lock

                //after inlining it must look like this
/*              {
                    //if ownership is moved
                    guardianT guardian = std::move(outerGuardian);
                    auto result = findBy(index);
                }
                or
                {
                    //if no ownership
                    auto result = findBy(index);
                }
*/
                if constexpr (std::is_rvalue_reference<LockT&&>::value) {
                    LockT guardian = std::move(_g);
                    return findBy(index);
                } else {
                    return findBy(index);
                }

                //alternate way that may produce additional var
                //using vType = typename std::conditional<std::is_rvalue_reference<LockT&&>::value, LockT, LockT&>::type;
                //[[maybe_unused]]  vType lock = std::forward<LockT>(_g);
            }

            template<class indexT, class LockT>
            inline auto remove(const indexT& index, [[maybe_unused]] LockT&& _g) {
                if constexpr (std::is_rvalue_reference<LockT&&>::value) {
                    LockT guardian = std::move(_g);
                    return removeBy(index);
                } else {
                    return removeBy(index);
                }
            }

            virtual std::string generateSID(const request* context = nullptr) {
                auto generator = hwrandom<unsigned>();
                return randomString<hwrandom<unsigned>>(10, generator);
            }

            virtual uint32_t index() {
                return _generator();
            }

            virtual manager::session_type* makeSession(const request* context = nullptr, int64_t timestamp = 0) {
                return new TSession(generateSID(context), index(), timestamp ? timestamp : esp_timer_get_time());
            }

            virtual bool validateSession(std::shared_ptr<iSession>& session, const request* context = nullptr) const {
                return true;
            }

            virtual void invalidateSessionPtr(session_ptr_type& sessionPtr, int reason = 0) const {
                std::static_pointer_cast<TSession>(sessionPtr)->invalidate(reason);
            }

            virtual void updateSessionPtr(session_ptr_type& sessionPtr, int64_t timestamp) const {
                std::static_pointer_cast<TSession>(sessionPtr)->update(timestamp);
            }

            virtual void renewSessionPtr(session_ptr_type& sessionPtr, const iSession::sid_type& sid, int64_t timestamp) const {
                std::static_pointer_cast<TSession>(sessionPtr)->renew(sid, timestamp);
            }

        public:

            static constexpr uint32_t TRAIT_ID = (uint32_t)traits::MANAGER;

            static constexpr size_t total =  SESSION_MAX_CLIENT_COUNT;

            typedef manager_iterator<false> iterator;
            typedef manager_iterator<true>  const_iterator;

            enum class closeReason : int {
                NORMAL_TERM = 0,
                EXPIRED,
                GENERAL_ERROR,
                TOTAL,
            };

            auto operator=(manager&) = delete;

            virtual ~manager() {
                auto guardian = guard<rw_lock_type>();
                for (int i = 0; i < storage.data.size(); ++i) {
                    if (storage.data[i] != nullptr) {
                        storage.data[i] = nullptr;
                        manager::invalidateSessionPtr(storage.data[i], (int)closeReason::NORMAL_TERM);
                    }
                }
                storage.count = 0;
            }

            //thread safe
            result_type open(const request* context = nullptr) override {
                auto timestamp = esp_timer_get_time();
                for (int i = 0; i < 3; ++i) {
                    manager::session_ptr_type sess(makeSession(context, timestamp));
                    if (auto result = place(sess, timestamp, guard<rw_lock_type>()); result) {
                        //invalidate expired session whose place we took
                        if (std::holds_alternative<manager::session_ptr_type>(result)) {
                            invalidateSessionPtr(std::get<manager::session_ptr_type>(result));
                        }
                        return sess;
                    } else {
                        invalidateSessionPtr(sess); //invalidate temporal session
                        if constexpr (LOG_SESSION) {
                            error("session placement error", result.code(), " ", sess->sid().c_str(), " ", i);
                        } else {
                            if (result.code() == NO_SLOT_SESSION_ERROR) {
                                error("session no slots", result.code(), " ", sess->sid().c_str(), " ", i);
                            }
                        }
                    }
                }
                return (esp_err_t)COLLISION_SESSION_ERROR;
            }

            //thread safe
            result_type open(const std::string& sid, const request* context = nullptr) override {
                auto timestamp = esp_timer_get_time();
                session_ptr_type sess;
                {
                    auto guardian = guard<ro_lock_type>();
                    if (sess = find(sid, guardian); sess != nullptr) {
                        if (validateSession(sess, context)) {
                            if (!sess->expired(timestamp)) {
                                updateSessionPtr(sess, timestamp);
                                return sess;
                            }
                        } else {
                            errorIf(LOG_SESSION, "invalid session", sess->sid().c_str());
                            return INVALID_SESSION_ERROR;
                        }
                    } else {
                        return ESP_ERR_NOT_FOUND;
                    }
                }
                //reach it only if session expired
                //check if invariant
                if (auto removed = remove(sess, guard<rw_lock_type>()); sess == removed) {
                    //we must delete exactly the same pointer
                    invalidateSessionPtr(sess, (int)closeReason::EXPIRED);
                    infoIf(LOG_SESSION, "session expired", sess->sid().c_str());
                }
                return ESP_ERR_NOT_FOUND;
            }

            //thread safe
            result_type renew(session_ptr_type& sess, const request* context = nullptr) override {
                auto sid = generateSID(context);
                auto guardian   = guard<rw_lock_type>();
                for (int i = 0; i < 3; ++i) {
                    if (find(sid, guardian) == nullptr) {
                        renewSessionPtr(sess, sid, esp_timer_get_time());
                        return sess;
                    }
                }
                return COLLISION_SESSION_ERROR;
            }

            //thread safe
            resBool close(const std::string& sid) override {
                if (storage.count > 0) { //suppose that read and write are atomic
                    if (auto sess = remove(sid, guard<rw_lock_type>()); sess != nullptr) {
                        //at this point no way to get another ptr to removed session, it no wat to prolong it
                        invalidateSessionPtr(sess, (int)closeReason::NORMAL_TERM);
                        return ESP_OK;
                    }
                }
                return ESP_ERR_NOT_FOUND;
            }


            //thread safe
            result_type fromIndex(index_type index, bool markAlive = false) override {
                auto timestamp = esp_timer_get_time();
                session_ptr_type sess;
                {
                    auto guardian = guard<ro_lock_type>();
                    if (sess = find(index, guardian); sess != nullptr) {
                        if (!sess->expired(timestamp)) {
                            if (markAlive) {
                                updateSessionPtr(sess, timestamp);
                            }
                            return sess;
                        }
                    } else {
                        return ESP_ERR_NOT_FOUND;
                    }
                }
                //reach it only if session expired
                //check if invariant
                if (auto removed = remove(sess, guard<rw_lock_type>()); sess == removed) {
                    //we must delete exactly the same pointer
                    invalidateSessionPtr(sess, (int) closeReason::EXPIRED);
                    infoIf(LOG_SESSION, "session expired", sess->sid().c_str());

                }
                return ESP_ERR_NOT_FOUND;
            }

			void walk(const walker_type& walker) final override {
				auto guard = sharedGuardian();
				for (auto it = begin(); it != end(); ++it) {
					if (!walker(*it, (*it)->index())) {
						return;
					}
				}
			}

            //thread safe
            size_t count() const final {
                if (storage.count > 0) {
                    auto timestamp = esp_timer_get_time();
                    size_t count = 0;
                    auto guardian = guard<ro_lock_type>();
                    for (size_t i = 0; i < total; ++i) {
                        if (storage.data[i] != nullptr && !storage.data[i]->expired(timestamp)) {
                            ++count;
                        }
                    }
                    return count;
                }
                return 0;
            }

            size_t size() const  {
                return storage.count;
            }

            //thread safe
            //close compatible
            void collect() override {
                std::vector<std::shared_ptr<iSession>> pending = {};
                if (storage.count > 0) {
                    auto timestamp = esp_timer_get_time();
                    auto guardian = guard<rw_lock_type>();
                    for (size_t i = 0; i < total; ++i) {
                        if (storage.data[i] != nullptr && storage.data[i]->expired(timestamp)) {
                            debugIf(LOG_SERVER_GC, storage.data[i]->sid().c_str(), " expired and collected: i: ", storage.data[i]->index());
                            pending.emplace_back(storage.data[i]);
                            storage.data[i] = nullptr;
                            storage.count--;
                        }
                    }
                }
                if (!pending.empty()) {
                    info("server gc collected", pending.size(), " sessions");
                    for (auto it = pending.begin(); it != pending.end(); ++it) {
                        invalidateSessionPtr(*it, (int)closeReason::EXPIRED);
                    }
                }
            }

            void* neighbour(uint32_t traitId) override {
                switch (traitId) {
                    case iManager::TRAIT_ID:
                        return static_cast<iManager*>(static_cast<manager<TSession>*>(this));
                    case TRAIT_ID:
                        return static_cast<manager<TSession>*>(this);
                    default:
                        return nullptr;
                }
            }

            void* downcast() override {
                return static_cast<manager<TSession>*>(this);
            }

            inline auto sharedGuardian() const {
                return guard<ro_lock_type>();
            }

            iterator begin() {
                return iterator(storage.data.begin(), storage.data.end(), esp_timer_get_time());
            }

            iterator end() {
                return iterator(storage.data.end(), storage.data.end(), esp_timer_get_time());
            }

            const_iterator begin() const {
                return const_iterator(storage.data.begin(), storage.data.end(), esp_timer_get_time());
            }

            const_iterator end() const {
                return const_iterator(storage.data.end(), storage.data.end(), esp_timer_get_time());
            }

    };

}
