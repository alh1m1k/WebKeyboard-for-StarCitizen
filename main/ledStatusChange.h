#pragma once

#include "generated.h"

#include <sys/_stdint.h>
#include <string>
#include <type_traits>

#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "util.h"
#include "packers.h"


using namespace std::literals;

#define IS_NOSCHEDULER  	  std::is_same<SCHEDULER, ledStatusChangeNoScheduler>::value
#define ENABLE_IF_NOSCHEDULER typename std::enable_if< std::is_same<T, ledStatusChangeNoScheduler>::value>::type* = 0
#define ENABLE_IF_SCHEDULER   typename std::enable_if<!std::is_same<T, ledStatusChangeNoScheduler>::value>::type* = 0

class ledStatusChangeNoScheduler {};

template<class CTRL, class NOTIFICATOR, typename COUNTER, class SCHEDULER = ledStatusChangeNoScheduler>
class ledStatusChange {
	
	CTRL& 			ctrl;
	NOTIFICATOR& 	notificator;
	COUNTER&        counter;
	
	std::conditional<IS_NOSCHEDULER, SCHEDULER, SCHEDULER&>::type scheduler;
	
	struct led {
		std::string name;
		uint8_t 	bitmask;
	};
		
	public:
		
		inline static const std::string onStatus  = "switched-on";
		inline static const std::string offStatus = "switched-off";
		
		//why this allow to be happened but not static constexpr std::string onStatus, class member vs class init?
        //there was problem with const expr std::string move to std::string_view
		static constexpr struct led leds[5] = {
			{ .name = "numlock", 	.bitmask = hid_keyboard_led_bm_t::KEYBOARD_LED_NUMLOCK  	},
			{ .name = "capslock", 	.bitmask = hid_keyboard_led_bm_t::KEYBOARD_LED_CAPSLOCK 	}, 
			{ .name = "scrolllock",	.bitmask = hid_keyboard_led_bm_t::KEYBOARD_LED_SCROLLLOCK  	}, 
			{ .name = "compose", 	.bitmask = hid_keyboard_led_bm_t::KEYBOARD_LED_COMPOSE  	}, 
			{ .name = "kana", 		.bitmask = hid_keyboard_led_bm_t::KEYBOARD_LED_KANA  		}, 
		};
		
		template<typename T = SCHEDULER>
		ledStatusChange(CTRL& ctrl, NOTIFICATOR& note, COUNTER& ctr, ENABLE_IF_NOSCHEDULER) noexcept: ctrl(ctrl), notificator(note), counter(ctr) {};
	
		template<typename T = SCHEDULER>
		ledStatusChange(CTRL& ctrl, NOTIFICATOR& note, COUNTER& ctr, SCHEDULER& scheduler, ENABLE_IF_SCHEDULER) noexcept: ctrl(ctrl), notificator(note), counter(ctr), scheduler(scheduler) {};
	
		void operator()(uint8_t current, uint8_t prev) const {
			
			uint8_t changed = prev ^ current;
			
			for (auto i = 0; i < 5; i++) {
				if ((changed & leds[i].bitmask) == leds[i].bitmask) {
					if ((current & leds[i].bitmask) == leds[i].bitmask) {
						scheduleNotify(leds[i].name, onStatus);
					} else {
						scheduleNotify(leds[i].name, offStatus);
					}
				}
			}
			
		}
		
		inline void scheduleNotify(const std::string& name, const std::string& status) const {
			if constexpr (IS_NOSCHEDULER) {
				debugIf(LOG_LED, "direct", name, " ", status);
				notify(name, status);
			} else {
				scheduler.schedule([&name, &status, this] -> void {
					debugIf(LOG_LED, "delayed", name, " ", status);
					this->notify(name, status);
				}, 0, 1);
			}
		}
		
		inline void notify(const std::string& name, const std::string& status) const {
			debugIf(LOG_LED, "ledStatusChange fixme", name, " ", status);
			ctrl.nextState(name, std::string(status));
			notificator.notify(kbNotify(counter(), name, status));
		}
};