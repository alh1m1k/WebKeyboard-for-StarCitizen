#pragma once

#include <cstddef>
#include <stddef.h>
#include <unordered_map>
#include <shared_mutex>
#include <iterator>
#include  <algorithm>

#include "bad_api_call.h"
#include "not_found.h"
#include "not_implemented.h"
#include "util.h"

using namespace std::literals;

template<typename KEY, typename VALUE>
class ctrlmap {
	
	using size_type = std::vector<std::string>::size_type;
	
	std::unordered_map<KEY, size_type> m = {};
	std::vector<VALUE> v = {};
	
	mutable std::shared_mutex rwLock = {};
	
	
	size_type index(const VALUE& value) {
		if (auto it = std::find(v.begin(), v.end(), value); it == v.end()) {
			v.push_back(std::move(value));
			return v.size() - 1;
		} else {
			return std::distance(v.begin(), it); //it is in range [0, max+1) so it real index not size
		}
		return 0;
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
				//debug("it", it->first, index[it->second]);
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
		
		typedef KEY 	key;
		typedef VALUE 	value;
			
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
		
		void nextState(const KEY& key, VALUE&& value) {
			if (value == "click"s || value.starts_with("activate-"sv)) {
				return;
			}
			std::unique_lock guardian(rwLock);
			if (value == "switched-off"s) {
				m.erase(key);
				return;
			}
			m[key] = index(value);
		}	
		
		//in case we want to move both key and value
		void nextState(KEY&& key, VALUE&& value) {
			if (value == "click"s || value.starts_with("activate-"sv)) {
				return;
			}
			std::unique_lock guardian(rwLock);
			if (value == "switched-off"s) {
				m.erase(key);
				return;
			}
			m[key] = index(value);
		}
};