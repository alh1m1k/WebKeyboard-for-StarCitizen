#pragma once

#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"

namespace syncing {

/*
 * TODO: test cover
 * 1: save/restore interrupt state
 * 2: save/restore interrupt state inside other critical section and wise versa
 * 3: locking unlocking, read/write locking
 * 4: try_locking
 */

/*
 * This class c++ representation of freertos taskENTER_CRITICAL&taskEXIT_CRITICAL functions
 * With difference that this spinlock implements a shared_lock interface.
 * It allows an arbitrary number of concurrent reads but synchronizes writes and writes with reads.
 * The time spent in the lock should be minimal, no IO operations (the task became active when waiting)
 * Use this class only to synchronize reading and writing inside ram memory.
 * Incorrect use of the class may result in a device stall/reboot.
 *
 * do not use this lock inside critical section or inside other same lock
 * it's does not support nested lock for simplicity
 */
//#warning "experimental critical_shared syncing in use"
class critical_shared {

	//this is reimplementation of freertos function taskENTER_CRITICAL ability to preserve
	//interrupt state, there is no way to access that storage.
	static inline BaseType_t port_uxInterruptState[portNUM_PROCESSORS] = {};

	std::atomic<int> locker = 0;

	public:
		typedef std::atomic<int>& native_handle_type;

		critical_shared() = default;
		critical_shared( const critical_shared& ) = delete;

		inline void lock() {
			//no nested lock allowed inside c++, so it simplified.
			//optionally xPortInterruptedFromISRContext() may be used to check that isr is disabled
			//but this will complicate things, also name of method shows that such use was not intended so it may change.
			port_uxInterruptState[xPortGetCoreID()] = portSET_INTERRUPT_MASK_FROM_ISR();
			for (;;) {
				int expected = 0;
				if (locker.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
					return;
				}
			}
		}

		bool try_lock() {
			port_uxInterruptState[xPortGetCoreID()] = portSET_INTERRUPT_MASK_FROM_ISR();
			int expected = 0;
			if (locker.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
				return true;
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(
				port_uxInterruptState[xPortGetCoreID()]
			);
			return false;
		}

		inline void unlock() {
			locker.store(0, std::memory_order_release);
			portCLEAR_INTERRUPT_MASK_FROM_ISR(
				port_uxInterruptState[xPortGetCoreID()]
			);
		}

		inline void lock_shared() {
			port_uxInterruptState[xPortGetCoreID()] = portSET_INTERRUPT_MASK_FROM_ISR();
			for (auto current = locker.load(std::memory_order_relaxed); true; current = locker.load(std::memory_order_relaxed)) {
				if (current >= 0) {
					if (locker.compare_exchange_weak(current, current+1, std::memory_order_acquire, std::memory_order_relaxed)) {
						return;
					}
				}
			}
		}

		bool try_lock_shared() {
			port_uxInterruptState[xPortGetCoreID()] = portSET_INTERRUPT_MASK_FROM_ISR();
			if (auto current = locker.load(std::memory_order_relaxed); current >= 0) {
				if (locker.compare_exchange_weak(current, current+1, std::memory_order_acquire, std::memory_order_relaxed)) {
					return true;
				}
			}
			portCLEAR_INTERRUPT_MASK_FROM_ISR(
				port_uxInterruptState[xPortGetCoreID()]
			);
			return false;
		}

		inline void unlock_shared() {
			locker.fetch_sub(1, std::memory_order_release);
			portCLEAR_INTERRUPT_MASK_FROM_ISR(
				port_uxInterruptState[xPortGetCoreID()]
			);
		}

		native_handle_type native_handle() {
			return locker;
		}
};

}