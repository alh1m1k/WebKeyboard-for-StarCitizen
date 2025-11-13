#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <iterator>
#include <algorithm>
#include <limits>

#include "bad_api_call.h"
#include "not_found.h"
#include "not_implemented.h"
#include "util.h"

using namespace std::literals;

template<typename KEY, typename VALUE>
class ctrlmap {
	
	using size_type = std::vector<std::string>::size_type;

    enum class stateOp {
        NO_OP,
        PUSH,
        POP,
    };
	
	std::unordered_map<KEY, size_type> m = {};
	std::vector<VALUE> v = {};
    ssize_t collectThreshold = 25;
	
	mutable std::shared_mutex rwLock = {};

	size_type index(VALUE&& value) {
		if (auto it = std::find(v.begin(), v.end(), value); it == v.end()) {
			v.push_back(std::move(value));
			return v.size() - 1;
		} else {
			return std::distance(v.begin(), it); //it is in range [0, max+1) so it real index not size
		}
		return 0;
	}

    inline void deindex(const VALUE& value) { /*this is noop*/ }

    //template helper
    stateOp actionOpType(const VALUE& value) {
        if (value == "click"sv || value.starts_with("activate-"sv)) {
            return stateOp::NO_OP;
        }
        if (value == "switched-off"sv) {
            return stateOp::POP;
        }
        return stateOp::PUSH;
    }
	
	class ctrlmap_iterator {
		
		std::unordered_map<KEY, size_type>::const_iterator it;
		const std::vector<VALUE>& index;
		
		typedef std::unordered_map<KEY, VALUE> mimicT;
		
		public:
			
	    	using difference_type 	= mimicT::difference_type;
	    	using value_type 		= mimicT::value_type;
	    	using pointer 			= mimicT::pointer;
	    	using reference 		= mimicT::reference;
	    	//using iterator_category = mimicT::iterator_category;
		
			ctrlmap_iterator(std::unordered_map<KEY, size_type>::const_iterator&& it, const std::vector<VALUE>& index): it(it), index(index) {};
			
			const value_type operator*() const {
				return std::make_pair(std::ref(it->first), std::ref(index[it->second]));
			} 
			/*
			* there is obviously problem with operator-> as no place in memory to point
			* unless to ctrlmap_iterator private member but in that case pointer address will not change between increment 
			* but value in that address will   
			*/
			pointer operator->() const {
				return not_impleneted();
			} 
	        auto operator++() {
				++it;
				return *this;
			}
	        bool operator==(const ctrlmap_iterator& other) const { 
				return other.it == it;
			}
	        bool operator!=(const ctrlmap_iterator& other) const {
				return other.it != it;
			}
	        bool operator>(const ctrlmap_iterator& other) {
				return other.it > it;
			}
	};
	
	public:
	
		using const_iterator = ctrlmap_iterator;
		using iterator 		 = ctrlmap_iterator;
		
		typedef KEY 	key_type;
		typedef VALUE 	value_type;

        static constexpr ssize_t collectThresholdSteep = 25;
			
		void lockShared() const {
			rwLock.lock_shared();
		}
		
		void unlockShared() const {
			rwLock.unlock_shared();
		}
		
		auto sharedGuardian() const { // why auto&& produce: warning: returning reference to temporary [-Wreturn-local-addr]
			return std::shared_lock(rwLock);
		}
				
		bool has(const KEY& key) const {
			std::shared_lock guardian(rwLock);
			return m.contains(key);
		}
				
		const_iterator begin() const {
			return const_iterator(m.begin(), v);
		}
		
		const_iterator end() const {
			return const_iterator(m.end(), v);
		}
		
		size_t count() const {
			std::shared_lock guardian(rwLock);
			return m.size();
		}

        template<class KeyRefT>
		void nextState(KeyRefT&& key, VALUE&& value) {
			std::unique_lock guardian(rwLock);
            switch (actionOpType(value)) {
                case stateOp::PUSH:
                    m[std::forward<KeyRefT>(key)] = index(std::move(value));
                    break;
                case stateOp::POP:
                    m.erase(key);
                    deindex(value);
                    break;
                case stateOp::NO_OP:
                default:
                    break;
            }
		}

        void collect() {
            std::unique_lock guardian(rwLock);
            ssize_t size = v.size();
            if (size > collectThreshold) {
                ssize_t vi = 0;
                for (auto it = m.begin(); it != m.end(); ++it) {
                    v[it->second = vi++] = std::move(v[it->second]);
                }
				if constexpr (LOG_SERVER_GC) {
					info("collected ctrlmap", size - vi);
				}
                v.resize(size = vi);
                if (size > collectThreshold) {
                    collectThreshold = std::min(std::numeric_limits<ssize_t>::max(), collectThreshold + collectThresholdSteep);
                } else if (size < (collectThreshold - collectThresholdSteep)) {
                    collectThreshold -= collectThresholdSteep;
                }

            }
        }

};