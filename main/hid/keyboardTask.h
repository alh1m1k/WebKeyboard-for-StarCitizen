#pragma once 

#include <functional>
#include <sys/_stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "esp_err.h"
#include <algorithm>

#include "generated.h"

#include "result.h"
#include "task.h"
#include "esp_timer.h"
#include "util.h"
#include "../exception/bad_api_call.h"

#define DEBUG_CYCLE

namespace hid {
	
	
	class keyboardTask {
				
		template<typename ITEM>
		class queue {
			
			friend keyboardTask;
						
			QueueHandle_t hndl = NULL;
					
			public:
			
				TickType_t waitForDeviceMs = 100;
				
				queue() {
					debugIf(LOG_KEYBOARD, "queue::queue");
					hndl = xQueueCreate(50, sizeof(ITEM));
					if (hndl == nullptr) {
						throw bad_api_call("queue::queue", ESP_FAIL);
					}
				};
				
				~queue(){
					debugIf(LOG_KEYBOARD, "queue::~queue");
					vQueueDelete(hndl);
				};	
								
				queue(queue&) = delete;
				
				queue& operator=(queue&) = delete;
		};
		
		struct report {
			uint8_t data[6] 	= {0, 0, 0, 0, 0, 0};
			uint8_t modifier 	= 0;
			uint8_t flags 		= 0;
		};
		
		
		queue<task> 	inputQueue;
		
		report activeReport;
		
		report combination; 
		
		report danglingKeys;
		  
		
		TaskHandle_t  hndl 		= NULL;
		
		std::vector<report> danglingReports = {};
			
		int64_t lastKeyPressUS = 0;
		
		uint32_t receivedPacketCounter = 0;
		uint32_t processedPacketCounter = 0;
		
		uint16_t nextSpacerDelayMs = 0;
		
		bool inCombination 	  = false;
		bool usingDanglingKeys  = false;
		bool hasAnyDangling = false;
		bool valid 			  = true;

		
		static constexpr uint8_t SPACER 			= (uint8_t)task::pressType::INVALID + 1;
		static constexpr uint8_t DOUBLETAP_SPACER 	= (uint8_t)task::pressType::INVALID + 2;
		static constexpr uint8_t SHORT_SPACER 		= (uint8_t)task::pressType::INVALID + 3;
		
		static void dumbCombination(const char* msg, const report& combination){
			debugIf(LOG_KEYBOARD, msg, 
				(uint)combination.data[0], 	" ", 
				(uint)combination.data[1], 	" ",
				(uint)combination.data[2], 	" ", 
				(uint)combination.data[3], 	" ", 
				(uint)combination.data[4], 	" ",
				(uint)combination.data[5], 	" ", 
				(uint)combination.modifier, " ", 
				(uint)combination.flags
			);
		}
		
		static bool putKeyInReport(uint8_t charCode, report& combination) {
			if (!charCode) {
				return false;
			}
			if (auto i = findKeyInReport(0, combination); i != -1) {
				combination.data[i] = charCode;
				return true;
			}	
			return false;
		}
		
		static int8_t findKeyInReport(uint8_t charCode, report& combination) {
			for (uint8_t i = 0; i < 6; i++) {
				if (combination.data[i] == charCode) {
					return i;
				}
			}
			return -1;
		}
		
		static bool hasAnyInReport(report& combination) {
			for (uint8_t i = 0; i < 6; i++) {
				if (combination.data[i] != 0) {
					return true;
				} else {
					return false;
				}
			}
			return false;
		}
		
		static bool mergeReports(report& target, const report& source, bool append = true) {
			for (uint8_t i = 0; i < 6; i++) {
				if (append) {
					if (findKeyInReport(source.data[i], target) == -1) {
						if (!putKeyInReport(source.data[i], target)) {
							return false;
						}
					}
				} else {
					if (auto idx = findKeyInReport(source.data[i], target); idx != -1) {
						target.data[i] = 0;
					}
				}
			}
			
			if (append) {
				target.modifier |=  source.modifier;
			} else {
				target.modifier &= ~source.modifier;
				uint8_t idx = 0;
				for (uint8_t i = 0; i < 6; i++) {
					if (target.data[i] != 0) {
						target.data[idx++] = target.data[i];
					}
				}
			}
			return true;
		}
		
		static bool reportsEqual(const report& first, const report& second) {
			return (*(uint32_t*)&first.data[0] == *(uint32_t*)&second.data[0]) &&
				   (*(uint16_t*)&first.data[4] == *(uint16_t*)&second.data[4]) &&
				   (first.modifier == second.modifier) && (first.flags == second.flags);
		}
		
		static bool reportsEqualKeys(const report& first, const report& second) {
			return (*(uint32_t*)&first.data[0] == *(uint32_t*)&second.data[0]) &&
				   (*(uint16_t*)&first.data[4] == *(uint16_t*)&second.data[4]) &&
				   (first.modifier == second.modifier);
		}
		
		
		protected:
		
			bool addDanglingKeys(const report& kbKey) {
				
				if (danglingReports.size() >= MAX_DANGLING_REPORTS) {
					return false;
				}
				
				if (std::find_if(danglingReports.begin(), danglingReports.end(), [&kbKey](const report& candidate) -> bool {
					return reportsEqualKeys(kbKey, candidate);
				}) != danglingReports.end()) {
					return false;
				}
								
				danglingReports.push_back(kbKey);
				
				if (mergeReports(danglingKeys, kbKey, true)) {
					hasAnyDangling = hasAnyInReport(danglingKeys);
					return true;
				}
				
				return false;
			}
			
			bool removeDanglingKeys(const report& kbKey) {
								
				if (danglingReports.size() == 0) {
					return false;
				}
												
				if (auto it = std::find_if(danglingReports.begin(), danglingReports.end(), [&kbKey](const report& candidate) -> bool {
					return reportsEqualKeys(kbKey, candidate);
				}); it == danglingReports.end()) {
					return false;
				} else {
					eraseByReplace(danglingReports, it);
				}
				
				danglingKeys = {};
				for (auto it = danglingReports.begin(), end = danglingReports.end(); it != end; ++it) {
					mergeReports(danglingKeys, *it, true);
				}
				
				hasAnyDangling = hasAnyInReport(danglingKeys);
				
				return true;
			}
		
			
			bool extractCombination(task kbKey, report& combination) {
				auto flags = (uint8_t)kbKey.flags()&(~(uint8_t)task::flag::MASK);
				switch (flags) {
					case (uint8_t)task::suffix::FORMAT_KB_KEY: //{modifiers, key, press, flag}
						debugIf(LOG_KEYBOARD, "FORMAT_KB_KEY", (int)kbKey[0],' ', (int)kbKey[1],' ', (int)kbKey[2]);
						combination.modifier |= kbKey[0];
						combination.flags 	  = kbKey[2];
						return putKeyInReport(kbKey[1], combination);
					case (uint8_t)task::suffix::FORMAT_KB_DEFAULT_PRESS: //{modifiers, key, key2, flag}
						debugIf(LOG_KEYBOARD, "FORMAT_KB_DEFAULT_PRESS", (int)kbKey[0],' ', (int)kbKey[1],' ', (int)kbKey[2]);
						combination.modifier |= kbKey[0];
						combination.flags 	  = (uint8_t)task::pressType::PRESS;
						return putKeyInReport(kbKey[1], combination) ? (putKeyInReport(kbKey[2], combination) || true) : false;		
					case (uint8_t)task::suffix::FORMAT_KB_ENDING_PRESS: //{key, key2, press, flag}
						debugIf(LOG_KEYBOARD, "FORMAT_KB_ENDING_PRESS", (int)kbKey[0], ' ',  (int)kbKey[1], ' ', (int)kbKey[2]);
						combination.flags 	  = kbKey[2];
						return putKeyInReport(kbKey[0], combination) ? (putKeyInReport(kbKey[1], combination) || true) : false;		
					case (uint8_t)task::suffix::FORMAT_KB_KEYS: //{key, key2, key3, flag}
						debugIf(LOG_KEYBOARD, "FORMAT_KB_KEYS", (int)kbKey[0],' ', (int)kbKey[1],' ', (int)kbKey[2]);
						return putKeyInReport(kbKey[0], combination) ? (putKeyInReport(kbKey[1], combination) ? (putKeyInReport(kbKey[2], combination) || true) : false ) : false;		
					default:
						error("extractCombination: undefined task format");
						return false;
				}

			}
			
			inline uint32_t applyEntropy(const uint32_t value) {
				auto e = entropy() * (value * 0.20);
				debugIf(LOG_KEYBOARD && LOG_ENTROPY, "delayGenerator", value, " ", e, " ", value + e);
				return value + e;
			}
		
			uint32_t generateDelay(uint8_t type) {
				switch (type) {
					case (uint8_t)task::pressType::LONGPRESS:
						return applyEntropy(timings.longpressMs);
					case (uint8_t)task::pressType::DOUBLETAP:
						return applyEntropy(timings.doubletapMs);
					case (uint8_t)task::pressType::SHORT:
						return applyEntropy(timings.shortPressMs);
					case DOUBLETAP_SPACER:
						return applyEntropy(timings.doubletapSpacerMs);
					case SHORT_SPACER:
						return applyEntropy(timings.shortPressSpacerMs);
					case SPACER:
						return applyEntropy(timings.spacerMs);
					default:
						return applyEntropy(timings.pressMs);
				}
			}
			
									
			bool execute(const report& kbKey) {
				debugIf(LOG_KEYBOARD, "execute in use");
				
				uint8_t flags = (uint8_t)kbKey.flags;
				bool handled = false;
								
				switch (flags) {
					case (uint8_t)task::pressType::LONGPRESS:
					case (uint8_t)task::pressType::PRESS:
					case (uint8_t)task::pressType::SHORT:
						keyPress(kbKey);
						usingDanglingKeys = false;
						handled = true;
						break;
					case (uint8_t)task::pressType::DOUBLETAP:
						keyPress(kbKey);
						waitMs(generateDelay(DOUBLETAP_SPACER));
						keyPress(kbKey);
						usingDanglingKeys = false;
						handled = true;
						break;
					case (uint8_t)task::pressType::DOWN:
						if (handled = addDanglingKeys(kbKey); handled) {
							if (usingDanglingKeys && hasAnyDangling) {
								keyDown(danglingKeys);
							}
						} else {
							error("fail2AddDanglingKeys");
						}
						break;
					case (uint8_t)task::pressType::UP:
						if (handled = removeDanglingKeys(kbKey); handled) {
							if (usingDanglingKeys ) {
								if (hasAnyDangling) {
									keyDown(danglingKeys);
								} else {
									keyUp(danglingKeys);
									usingDanglingKeys = false;
								}
							}
						} else {
							error("fail2RemoveDanglingKeys");
						}
						break;
					default:
						error("keyboardTask::press undefined flags", (int)kbKey.flags);

				}
								
				lastKeyPressUS = esp_timer_get_time();
				
				return handled;
			}
			
			bool type(const report& kbKey) {
				debugIf(LOG_KEYBOARD, "type in use");
	
				for (uint8_t i = 0; i < 6; i++) {
					if (kbKey.data[i]) {
						throttleMs(applyEntropy(nextSpacerDelayMs));
						if (!execute({{kbKey.data[i]}, kbKey.modifier, kbKey.flags})) {
							return false;
						} else {
							nextSpacerDelayMs = (uint8_t)kbKey.flags == (uint8_t)task::pressType::SHORT ? timings.shortPressSpacerMs : timings.spacerMs;
						}
					} else {
						break;
					}
				}
				
				debugIf(LOG_KEYBOARD,"type done");
				
				return true;
				
			}
			
			inline void keyUp(const report& kbKey) {
				tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL);
				dumbCombination("keyup", combination);
			}
			
			inline void keyDown(const report& kbKey) {
				tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, kbKey.modifier, kbKey.data);
				dumbCombination("keydown", combination);
				activeReport = kbKey;
			}
			
			inline void keyPress(const report& kbKey, uint32_t delay = 0) {
				delay = delay ? delay : generateDelay(kbKey.flags);
				keyDown(kbKey);
				waitMs(delay);
				debugIf(LOG_KEYBOARD, "keyPress delay", delay);
				keyUp(kbKey);
			}
			
			inline void throttleMs(const uint32_t throttle) {
				if (auto left = (esp_timer_get_time() - lastKeyPressUS) / 1000; left < throttle) {
					debugIf(LOG_KEYBOARD, "throttleMs", throttle-left);
					waitMs(throttle-left);
				}
			}
			
			void waitMs(const uint32_t ms) {
				int64_t startTime = esp_timer_get_time();
				int64_t us 		  = std::max((int64_t)ms * 1000 - 500, (int64_t)0);
				vTaskDelay(pdMS_TO_TICKS(ms));
				//now we are in 1hz range of target time, by default it's 10ms 
				int64_t startYeld = esp_timer_get_time();
				while (esp_timer_get_time() - startTime < us) {
					taskYIELD();
				}
				//now we are before time ~500us or after target time with some dT
				debugIf(LOG_KEYBOARD, "waitMs us:", ms * 1000, " actual: ", esp_timer_get_time() - startTime, " yelding: ", esp_timer_get_time() - startYeld);
			}
		
		public:
		
			typedef std::function<float()> entropy_generator_type;
			typedef result<uint32_t>       push_result;
		
			entropy_generator_type entropy = ([]() -> float { return 0.0; });
						
			static const auto MAX_DANGLING_REPORTS = 8;		
			
			struct {
				uint16_t 	deviceWaitMs 		= 250;
				uint16_t 	pressMs 		   	= 76; //+- 20
				uint16_t 	longpressMs 		= 500;
				uint16_t    spacerMs            = 200;
				uint16_t 	doubletapMs 		= 70;
				uint16_t 	doubletapSpacerMs 	= 70;
				uint16_t    shortPressMs        = 50;
				uint16_t    shortPressSpacerMs  = 50;
			} timings;
			
			keyboardTask(): inputQueue() {
				debugIf(LOG_KEYBOARD, "keyboardTask::keyboardTask");
				auto status = xTaskCreate([](void* o){
					while (true) {
						(*static_cast<keyboardTask*>(o))();
					}
				}, "keyboardTask", 4096, this, 10, &hndl);
				if (status != pdPASS) {
					throw bad_api_call("keyboardTask::keyboardTask", ESP_FAIL);
				}
			};
			
			~keyboardTask(){
				debugIf(LOG_KEYBOARD, "keyboardTask::~keyboardTask");
				if (valid) {
					vTaskDelete(hndl);
				}
			};
			
			keyboardTask(keyboardTask&) = delete;
			
			keyboardTask& operator=(keyboardTask&) = delete;
			
			inline void operator()() {
#ifndef DEBUG_CYCLE 			
				//if (tud_mounted()) {
#endif
					uint32_t buffer;
					if(xQueueReceive(inputQueue.hndl, &buffer, (TickType_t)5)) {
						task kbKey(buffer);
						auto flags = kbKey.flags();
						debugIf(LOG_KEYBOARD, "keyboardTask::operator()", buffer, " ", (int)flags);
						if (flags&(uint8_t)task::flag::COMBINATION_END) { //also  flags&((uint8_t)task::flag::COMBINATION_END | (uint8_t)task::flag::COMBINATION_BEGIN)
							inCombination = false;
							extractCombination(kbKey, combination);
							dumbCombination("combination", combination);
							if (!execute(combination)) {
								//todo register error;
							}
							processedPacketCounter++;
						} else if (flags&(uint8_t)task::flag::COMBINATION_BEGIN) {
							inCombination = true;
							combination = {};
							extractCombination(kbKey, combination);
						} else if (inCombination) {
							extractCombination(kbKey, combination);
						} else {
							combination = {};
							extractCombination(kbKey, combination);
							dumbCombination("standalone", combination);
							if (type(combination)) {
								//todo register error;
							}
							processedPacketCounter++;
						}
					} else {
						if (hasAnyDangling && !usingDanglingKeys && (esp_timer_get_time() - lastKeyPressUS >= 25000)) {
							keyDown(danglingKeys);
							lastKeyPressUS = esp_timer_get_time();
							usingDanglingKeys = true;
							dumbCombination("danglingKeys", danglingKeys);
						}
					}
#ifndef DEBUG_CYCLE 
	/*			} else {
					waitMs(timings.deviceWaitMs);
				}*/
#endif
				
			}
								
			push_result push_back(const task kbValue) {
				debugIf(LOG_KEYBOARD, "push_back", kbValue.serialize());
				auto serialized = kbValue.serialize();
				if (xQueueSendToBack(inputQueue.hndl, &serialized, 0)) {
					return receivedPacketCounter++;
				} else {
					return ESP_FAIL;
				}
			}
			
			inline uint32_t receivedCnt() {
				return receivedPacketCounter;
			} 
			
			inline uint32_t processedCnt() {
				return processedPacketCounter;
			} 
		
	};
	
}