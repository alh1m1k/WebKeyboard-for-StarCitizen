#pragma once

#include <cstddef>
#include <stddef.h>
#include <array>
#include <shared_mutex>
#include <iterator>

#include "bad_api_call.h"
#include "not_found.h"
#include "util.h"

		

template<class T, size_t SIZE, typename ID = int>
class clients {
	
	struct record_s {
		T 	member = {};
		ID  id 	   = {};
		bool valid = false;
	};
	
	std::array<record_s, SIZE> storage = {};
	size_t _count = 0;
	
	mutable std::shared_mutex rwLock = {};
	
	record_s* first = &storage[0];
		
	class clients_iterator {
		
		record_s* cursor 		 = nullptr;
		const record_s* min 	 = nullptr;
		const record_s* max 	 = nullptr;
				
		public:
		
	    	using difference_type 	= std::ptrdiff_t;
	    	using value_type 		= T;
	    	using pointer 			= T*;
	    	using reference 		= T&;
	    	using iterator_category = std::bidirectional_iterator_tag;
			
	    	explicit clients_iterator(record_s* cursor, const record_s* min, const record_s* max): cursor(cursor), min(min), max(max){};
			clients_iterator(const clients_iterator& copy) = default;
	        clients_iterator(clients_iterator &&move) = default;
	        
	        clients_iterator& operator=(const clients_iterator& copy) {
				cursor = copy.cursor;
				min	   = copy.min;
				max	   = copy.max;
				
				return *this;
			}
	        value_type operator*() const {
				return cursor->member;
			} 
	        auto operator++() {
				/*size_t count = 0;*/
				do {++cursor; /*debug("++", count++, " ", cursor->valid, " ", (void*)cursor, " ", (void*)max);*/} while(cursor < max ? !cursor->valid : false);
				return *this;
			}
	        auto operator--() {
				/*size_t count = 0;*/
				do {--cursor; /*debug("--", count++, " ", (void*)cursor, " ", (void*)min);*/} while(cursor >= 0 ? !cursor->valid : false);
				return *this;
			}
	        bool operator==(const clients_iterator& other) const { 
				return other.cursor == this->cursor;
			}
	        bool operator!=(const clients_iterator& other) const {
				return other.cursor != this->cursor;
			}
	        bool operator>(const clients_iterator& other) {
				return other.cursor > this->cursor;
			}
	        ID id() {
				return cursor->id;
			}
	        auto& unsafeRef() {
				return cursor->member;
			}
	        auto& unsafeId() {
				return cursor->id;
			}
	};
	
	public:
	
		typedef clients_iterator 	iterator;
		typedef T 					member;
		typedef ID 					id;
		
		static constexpr auto size 	= SIZE;
		static constexpr ID ANY_ID = {}; //empty id is ANY, not trivial type probably won't compile
        //this is ugly and subject to change
		
		void lockShared() const {
			rwLock.lock_shared();
		}
		
		void unlockShared() const {
			rwLock.unlock_shared();
		}
		
		auto sharedGuardian() const { // why auto&& produce: warning: returning reference to temporary [-Wreturn-local-addr]
			return std::shared_lock(rwLock);
		}
		
		bool has(const T& client) const {
			std::shared_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeRef() == client) {
					return true;
				}
			}
			return false;
		}
		
		bool has(const ID& clientId) const {
			std::shared_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeId() == clientId) {
					return true;
				}
			}
			return false;
		}
		
		T get(const ID& clientId) const {
			std::shared_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeId() == clientId) {
					return *it;
				}
			}
			throw not_found("client not found");
		}
		
		ID get(const T& client) const {
			std::shared_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeRef() == client) {
					return it.id();
				}
			}
			throw not_found("client not found");
		}
		
		clients_iterator begin() const {
			//danger of conversion (const this)::class to (this)::class
			if (!_count) {
				return end();
			} else if (storage[0].valid) {
				return clients_iterator((record_s*)&storage[0], &storage[0], (&storage[SIZE-1])+1);
			} else {
				return ++clients_iterator((record_s*)&storage[0], &storage[0], (&storage[SIZE-1])+1); //find first
			}
		}
		
		clients_iterator end() const {
			auto max = &storage[SIZE-1] + 1;
			return clients_iterator((record_s*)max, &storage[0], (record_s*)max);
		}
		
		size_t count() const {
			std::shared_lock guardian(rwLock);
			return _count;
		} 
		
		bool add(T&& client, ID id) {
			std::unique_lock guardian(rwLock);
			for (auto i = 0; i < size; ++i) {
				if (!storage[i].valid) {
					storage[i].valid = true;
					storage[i].member = client;
					storage[i].id = id;
					_count++;
					return true;
				}
			}
			return false;
		} 
		
		bool add(T&& client, ID&& id) {
			
			std::unique_lock guardian(rwLock);
			for (auto i = 0; i < size; ++i) {
				if (!storage[i].valid) {
					storage[i].valid = true;
					storage[i].member = client;
					storage[i].id = id;
					_count++;
					return true;
				}
			}
			return false;
		} 
		
		bool remove(const T& client) {
			std::unique_lock guardian(rwLock);
			for (auto i = 0; i < size; ++i) {
				if (storage[i].valid && client == storage[i].member) {
					storage[i].valid = false;
					_count--;
					return true;
				}
			}
			return false;
		}
		
		bool remove(const ID& id) {
			std::unique_lock guardian(rwLock);
			for (auto i = 0; i < size; ++i) {
				if (storage[i].valid && id == storage[i].id) {
					storage[i].valid = false;
					_count--;
					return true;
				}
			}
			return false;
		}
		
		bool update(const ID& clientId, T&& client) {
			std::unique_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeId() == clientId) {
					it.unsafeRef() = client;
					return true;
				}
			}
			return false;
		}
		
		bool update(T& client, ID& clientId) {
			std::unique_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeRef() == client) {
					it.unsafeId() = clientId;
					return true;
				}
			}
			return false;
		}
		
		bool update(T& client, ID&& clientId) {
			std::unique_lock guardian(rwLock);
			for (auto it = begin(), endIt = end(); it != endIt; ++it) {
				if (it.unsafeRef() == client) {
					it.unsafeId() = clientId;
					return true;
				}
			}
			return false;
		}
};