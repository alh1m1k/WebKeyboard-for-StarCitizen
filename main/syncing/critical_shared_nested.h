#pragma once

#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

namespace syncing {

extern const uint8_t port_interruptNesting_address[] 	asm("port_interruptNesting");
extern const uint8_t port_uxCriticalNesting_address[] 	asm("port_uxCriticalNesting");
extern const uint8_t port_uxOldInterruptState_address[] asm("port_uxOldInterruptState");

/**
 * @see syncing::critical_shared
 *
 * this is quick and dirty implementation of nested critical_shared
 * that able to be used inside critical_section and vise versa
 *
 */
class critical_shared_nested {

	//asm(".bss.port_interruptNesting")
	//static inline unsigned 	  *port_interruptNesting 	= reinterpret_cast<unsigned*>((int)port_interruptNesting_address);
	//asm(".bss.port_uxCriticalNesting")
	static inline BaseType_t  *port_uxCriticalNesting   = reinterpret_cast<BaseType_t*>((int)port_uxCriticalNesting_address);
	//asm(".bss.port_uxOldInterruptState")
	static inline BaseType_t  *port_uxOldInterruptState = reinterpret_cast<BaseType_t*>((int)port_uxOldInterruptState_address);

	std::atomic<int> locker = 0;

	inline void pushIntl() {
		BaseType_t xOldInterruptLevel = portSET_INTERRUPT_MASK_FROM_ISR();
		BaseType_t coreID = xPortGetCoreID();
		BaseType_t newNesting = port_uxCriticalNesting[coreID] + 1;
		port_uxCriticalNesting[coreID] = newNesting;
		//If this is the first entry to a critical section. Save the old interrupt level.
		if ( newNesting == 1 ) {
			port_uxOldInterruptState[coreID] = xOldInterruptLevel;
		}
	}

	inline void popIntl() {
		BaseType_t coreID = xPortGetCoreID();
		BaseType_t nesting = port_uxCriticalNesting[coreID];

		/* Critical section nesting count must never be negative */
		configASSERT( nesting > 0 );

		if (nesting > 0) {
			nesting--;
			port_uxCriticalNesting[coreID] = nesting;
			//This is the last exit call, restore the saved interrupt level
			if ( nesting == 0 ) {
				portCLEAR_INTERRUPT_MASK_FROM_ISR(port_uxOldInterruptState[coreID]);
			}
		}
	}

	public:
		typedef std::atomic<int>& native_handle_type;

		critical_shared_nested() = default;
		critical_shared_nested( const critical_shared_nested& ) = delete;

		void lock() {
			pushIntl();
			for (;;) {
				int expected = 0;
				if (locker.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
					return;
				}
			}
		}

		bool try_lock() {
			pushIntl();
			int expected = 0;
			if (locker.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
				return true;
			}
			popIntl();
			return false;
		}

		void unlock() {
			locker.store(0, std::memory_order_release);
			popIntl();
		}

		void lock_shared() {
			pushIntl();
			for (auto current = locker.load(std::memory_order_relaxed); true; current = locker.load(std::memory_order_relaxed)) {
				if (current >= 0) {
					if (locker.compare_exchange_weak(current, current+1, std::memory_order_acquire, std::memory_order_relaxed)) {
						return;
					}
				}
			}
		}

		bool try_lock_shared() {
			pushIntl();
			if (auto current = locker.load(std::memory_order_relaxed); current >= 0) {
				if (locker.compare_exchange_weak(current, current+1, std::memory_order_acquire, std::memory_order_relaxed)) {
					return true;
				}
			}
			popIntl();
			return false;
		}

		void unlock_shared() {
			locker.fetch_sub(1, std::memory_order_release);
			popIntl();
		}

		native_handle_type native_handle() {
			return locker;
		}
};

}