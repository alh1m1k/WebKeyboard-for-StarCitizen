#pragma once

#include "critical.h"
#include "deviceDescriptor.h"

#include <algorithm>
#include <functional>
#include "class/hid/hid_device.h"
#include "esp_err.h"
#include <sys/_stdint.h>

#include "generated.h"

#include "../exception/bad_api_call.h"
#include "asciiDictionary.h"
#include "esp_timer.h"
#include "ipc/queue.h"
#include "reportUnpacker.h"
#include "result.h"
#include "syncing/scoped_lock.h"
#include "tasking/task.h"

// #define DEBUG_CYCLE

namespace hid {
	
	
	class compositeTask {

		enum pendingFlush {
			KEYBOARD	= 0x01 << 0x00,
			JOYSTICK0	= 0x01 << 0x01,
			JOYSTICK1	= 0x01 << 0x02,
			MOUSE		= 0x01 << 0x03,
			TYPING		= 0x01 << 0x04,
		};

		template<typename T, typename LOCKER>
		class guardianRestrictor {
			T* subject;
			LOCKER locker;
			std::function<void()> flusher;
			public:
				guardianRestrictor(T* subject, LOCKER&& locker, const std::function<void()>& flusher)
					: subject(subject), locker(std::move(locker)), flusher(flusher) {};
				~guardianRestrictor() { flusher(); /*debug("release guardian");*/ };
				explicit operator bool() const noexcept {/* debug("guardian lock", (int)!!locker);*/ return  !!locker; }
				T* operator->() { return data(); }
				const T* operator->() const { return data(); }
				[[nodiscard]] void* data() { assert(!!locker);  return subject; }
				[[nodiscard]] const void* data() const { assert(!!locker);  return subject; }
				[[nodiscard]] constexpr size_t size() const noexcept { return sizeof(T); }
		};

		//represent data that not safe to be put as member
		//and should be "local" ie on "stack" to prevent bugs
		struct local_context_type {
			int lockedDev = 0;
		};

		using keyboard_report_type = hid_keyboard_report_t;

		struct TU_ATTR_PACKED joystick_report_type {
			uint16_t x;
			uint16_t y;
			uint16_t rx;
			uint16_t ry;
			uint16_t ls;
			uint16_t rs;
			uint16_t ld;
			uint16_t rd;
			uint32_t buttons;
		};

		//restricted ptr to joystick state used by direct mode
		struct TU_ATTR_PACKED joystick_direct_type {
			uint16_t x;
			uint16_t y;
			uint16_t rx;
			uint16_t ry;
			uint16_t ls;
			uint16_t rs;
			uint16_t ld;
			uint16_t rd;
		};

		struct joystick_state_type {
			joystick_report_type unsafe{
				AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE,
				AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIN, AXIS_MIN, 0
			};
			std::mutex mutex{};
		};

		using mouse_report_type = hid_abs_mouse_report_t;

		//restricted ptr to mouse state used by direct mode
		struct TU_ATTR_PACKED mouse_direct_type
		{
			int16_t  x;
			int16_t  y;
		};

		struct mouse_state_type {
			mouse_report_type unsafe{};
			std::mutex mutex{};
		};

		struct joystick_command_type {
			uint32_t buttons;
			uint8_t flags;
		};

		struct mouse_command_type {
			uint8_t buttons;
			int8_t  wheel;
			int8_t  pan;
			uint8_t flags{};
		};

		struct command {
			keyboard_report_type keyboard{};
			joystick_command_type joystick0{};
			joystick_command_type joystick1{};
			mouse_command_type mouse{};
			uint8_t flags = 0x00;
			uint8_t pendingFlush = 0x00;
		};

		struct {
			joystick_state_type joystick0;
			joystick_state_type joystick1;
			mouse_state_type mouse;
		} state;

		command current;

		::ipc::queue<report> queue;
		::tasking::task task;

		std::mutex flushMutex = {};

		std::vector<command> danglingCommands = {};

		int64_t lastKeyPressUS = 0;

		uint32_t receivedPacketCounter = 0;
		uint32_t processedPacketCounter = 0;

		uint16_t nextSpacerDelayMs = 0;

		bool usingDanglingKeys = false;
		bool hasAnyDangling = false;

		static constexpr uint8_t SPACER 			= (uint8_t)pressType::INVALID + 1;
		static constexpr uint8_t DOUBLETAP_SPACER 	= (uint8_t)pressType::INVALID + 2;
		static constexpr uint8_t SHORT_SPACER 		= (uint8_t)pressType::INVALID + 3;

		static bool hasAnyInCommand(const command& cmd) noexcept {
			return  cmd.pendingFlush != 0x00;
		}

		static bool mergeCommands(command& target, const command& source) {
			if ((source.pendingFlush&KEYBOARD) == KEYBOARD) {
				int i = 0, j = 0;
				while (target.keyboard.keycode[i] != 0x00 && i < 6) { ++i; }
				while (source.keyboard.keycode[j] != 0x00) {
					if (i >= 6) { return false; }
					target.keyboard.keycode[i++] = source.keyboard.keycode[j++];
				}
				target.keyboard.modifier |= source.keyboard.modifier;
			}
			if ((source.pendingFlush&JOYSTICK0) == JOYSTICK0) {
				target.joystick0.buttons |= source.joystick0.buttons;
			}
			if ((source.pendingFlush&JOYSTICK1) == JOYSTICK1) {
				target.joystick1.buttons |= source.joystick1.buttons;
			}
			if ((source.pendingFlush&MOUSE) == MOUSE) {
				target.mouse.buttons |= source.mouse.buttons;
				target.mouse.pan = target.mouse.wheel = 0;
			}
			target.pendingFlush |= source.pendingFlush;
			return true;
		}

		static bool commandsEqual(const command& first, const command& second) noexcept {
			return memcmp(&first, &second, sizeof(keyboard_report_type)) == 0;
		}

		static bool commandsEqualKeys(const command& first, const command& second) noexcept {
			return memcmp(first.keyboard.keycode, second.keyboard.keycode, 6) == 0 &&
				(first.keyboard.modifier == second.keyboard.modifier) &&
				(first.joystick0.buttons == second.joystick0.buttons) &&
				(first.joystick1.buttons == second.joystick1.buttons) &&
				(first.mouse.buttons == second.mouse.buttons) &&
				(first.mouse.wheel == second.mouse.wheel) &&
				(first.mouse.pan == second.mouse.pan);
		}

		static void dumbCombination(const char* msg, const command& combination) {
			infoIf(true, msg,
				" \n ",
				(uint)combination.keyboard.keycode[0], 	" ",
				(uint)combination.keyboard.keycode[1], 	" ",
				(uint)combination.keyboard.keycode[2], 	" ",
				(uint)combination.keyboard.keycode[3], 	" ",
				(uint)combination.keyboard.keycode[4], 	" ",
				(uint)combination.keyboard.keycode[5], 	" ",
				(uint)combination.keyboard.modifier, " ",
				(uint)combination.keyboard.reserved, " ",
				" \n ",
				(uint32_t)combination.joystick0.buttons, " ",
				(uint32_t)combination.joystick0.flags, " ",
				" \n ",
				(uint32_t)combination.joystick1.buttons, " ",
				(uint32_t)combination.joystick1.flags, " ",
				" \n ",
				(uint32_t)combination.mouse.buttons, " ",
				(int)combination.mouse.wheel, " ",
				(int)combination.mouse.pan, " ",
				(uint32_t)combination.mouse.flags, " ",
				" \n ",
				(uint32_t)combination.pendingFlush, " ",
				(uint32_t)combination.flags, " "
			);
		}

		protected:

			bool addDanglingCommand(const command& cmd) {
				//dumbCombination("addDanglingCommand", cmd);
				if (danglingCommands.size() >= MAX_DANGLING_REPORTS || !hasAnyInCommand(cmd)) {
					return false;
				}

				if (std::find_if(danglingCommands.begin(), danglingCommands.end(), [&cmd](const command& candidate) -> bool {
					return commandsEqualKeys(cmd, candidate);
				}) != danglingCommands.end()) {
					return false;
				}

				danglingCommands.push_back(cmd);

				return hasAnyDangling = true;
			}

			bool removeDanglingKeys(const command& cmd) {
				//dumbCombination("removeDanglingKeys", cmd);
				if (!hasAnyDangling) { return false; }
				if (const auto it = std::find_if(
					danglingCommands.begin(),
					danglingCommands.end(),
					[&cmd](const command& candidate) -> bool {
					return commandsEqualKeys(cmd, candidate);
				}); it == danglingCommands.end()) {
					return false;
				} else {
					eraseByReplace(danglingCommands, it);
				}
				hasAnyDangling = !danglingCommands.empty();
				return true;
			}

			static command getMergedDanglingCommand(const std::vector<command>& list) {
				command cmd = {};
				for (auto& source : list) {
					mergeCommands(cmd, source);
				}
				//("getMergedDanglingCommand", cmd);
				return cmd;
			}

			[[nodiscard]] inline uint32_t applyEntropy(const uint32_t value) const {
				const auto e = (int32_t)(entropy() * (value * 0.20));
				debugIf(LOG_USB_DEVIVE && LOG_ENTROPY, "delayGenerator", value, " ", e, " ", value + e);
				return value + e;
			}

			[[nodiscard]] uint32_t generateDelay(const uint8_t type) const {
				switch (type) {
					case (uint8_t)pressType::LONGPRESS:
						return applyEntropy(timings.longpressMs);
					case (uint8_t)pressType::DOUBLETAP:
						return applyEntropy(timings.doubletapMs);
					case (uint8_t)pressType::SHORT:
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

			bool executeCommand(const command& cmd) {
				bool handled = false;
				switch (cmd.flags) {
					case (uint8_t)pressType::UNSPECIFIED:
						flushPush(cmd);
						usingDanglingKeys = false;
						handled = true;
						break;
					case (uint8_t)pressType::LONGPRESS:
					case (uint8_t)pressType::PRESS:
					case (uint8_t)pressType::SHORT:
						pushAndRelease(cmd, generateDelay(cmd.flags));
						usingDanglingKeys = false;
						handled = true;
						break;
					case (uint8_t)pressType::DOUBLETAP:
						pushAndRelease(cmd, generateDelay(cmd.flags));
						waitMs(generateDelay(DOUBLETAP_SPACER));
						pushAndRelease(cmd, generateDelay(cmd.flags));
						usingDanglingKeys = false;
						handled = true;
						break;
					case (uint8_t)pressType::DOWN:
						if (handled = addDanglingCommand(cmd); handled) {
							if (usingDanglingKeys && hasAnyDangling) {
								flushPush(getMergedDanglingCommand(danglingCommands));
							}
						} else {
							error("fail2AddDanglingKeys");
						}
						break;
					case (uint8_t)pressType::UP:
						if (handled = removeDanglingKeys(cmd); handled) {
							if (usingDanglingKeys) {
								if (hasAnyDangling) {
									flushPush(getMergedDanglingCommand(danglingCommands));
								} else {
									flushRelease(cmd);
									usingDanglingKeys = false;
								}
							}
						} else {
							error("fail2RemoveDanglingKeys");
						}
						break;
					default:
						error("keyboardTask::executeCommand undefined flags", (int)cmd.flags);
				}

				lastKeyPressUS = esp_timer_get_time();

				return handled;
			}

			bool executeTyping(const std::string_view& text, const uint8_t flags, command buffer) {

				if constexpr (!keyboard_included_type::value) { return true; }

				debugIf(LOG_USB_DEVIVE, "type in use ", text);

				buffer.flags = buffer.keyboard.reserved = flags;
				buffer.pendingFlush = KEYBOARD;
				for (const char symbol : text) {
					throttleMs(applyEntropy(nextSpacerDelayMs));
					buffer.keyboard.modifier = 0;
					if (ASCII2KEYCODE[(uint8_t)symbol][0]) {
						buffer.keyboard.modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
					}
					const uint8_t keyCode = ASCII2KEYCODE[(uint8_t)symbol][1];
					buffer.keyboard.keycode[0] = keyCode;
					if (!executeCommand(buffer)) {
						return false;
					}
					nextSpacerDelayMs = (uint8_t)flags == (uint8_t)pressType::SHORT ? timings.shortPressSpacerMs : timings.spacerMs;
				}

				debugIf(LOG_USB_DEVIVE, "type done");

				return true;

			}

			void flushPush(const command& cmd) noexcept {
				//dumbCombination("flushPush", cmd);
				if constexpr (DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC) {
					return;
				}
				auto guardian = std::unique_lock(flushMutex);
				if constexpr (keyboard_included_type::value) {
					if (IS(cmd.pendingFlush, KEYBOARD)) {
						//tud_hid_keyboard_report()
						waitTransferCompletion();
						tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, cmd.keyboard.modifier, cmd.keyboard.keycode);
						if (ANY_EXCEPT(cmd.pendingFlush, KEYBOARD)) {
							//has any except keyboard itself
							//apply small delay to improve combination reading from OS
							waitMs(generateDelay(SHORT_SPACER));
						}
					}
				}
				if constexpr (mouse_included_type::value) {
					if (IS(cmd.pendingFlush, MOUSE)) {
						state.mouse.unsafe.pan = cmd.mouse.pan;
						state.mouse.unsafe.wheel = cmd.mouse.wheel;
						state.mouse.unsafe.buttons = cmd.mouse.buttons;
						waitTransferCompletion();
						tud_hid_report(HID_ITF_PROTOCOL_MOUSE,  &state.mouse.unsafe, sizeof(mouse_report_type));
					}
				}
				if constexpr (joystick0_included_type::value) {
					if (IS(cmd.pendingFlush, JOYSTICK0)) {
						state.joystick0.unsafe.buttons = cmd.joystick0.buttons;
						waitTransferCompletion();
						tud_hid_report(REPORT_ID_GAMEPAD0, &state.joystick0.unsafe, sizeof(joystick_report_type));
					}
				}
				if constexpr (joystick1_included_type::value) {
					if (IS(cmd.pendingFlush, JOYSTICK1)) {
						state.joystick1.unsafe.buttons = cmd.joystick1.buttons;
						waitTransferCompletion();
						tud_hid_report(REPORT_ID_GAMEPAD1, &state.joystick1.unsafe, sizeof(joystick_report_type));
					}
				}
			}

			void flushRelease(const command& cmd) noexcept {
				//dumbCombination("flushRelease", cmd);
				if constexpr (DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC) {
					return;
				}
				auto guardian = std::unique_lock(flushMutex);
				if constexpr (joystick0_included_type::value) {
					if (IS(cmd.pendingFlush, JOYSTICK0)) {
						state.joystick0.unsafe.buttons = 0x00;
						waitTransferCompletion();
						tud_hid_report(REPORT_ID_GAMEPAD0, &state.joystick0.unsafe, sizeof(joystick_report_type));
					}
				}
				if constexpr (joystick1_included_type::value) {
					if (IS(cmd.pendingFlush, JOYSTICK1)) {
						state.joystick1.unsafe.buttons = 0x00;
						waitTransferCompletion();
						tud_hid_report(REPORT_ID_GAMEPAD1, &state.joystick1.unsafe, sizeof(joystick_report_type));
					}
				}
				if constexpr (mouse_included_type::value) {
					if (IS(cmd.pendingFlush, MOUSE)) {
						state.mouse.unsafe.pan = state.mouse.unsafe.wheel = 0x00;
						state.mouse.unsafe.buttons = 0x00;
						waitTransferCompletion();
						tud_hid_report(HID_ITF_PROTOCOL_MOUSE,  &state.mouse.unsafe, sizeof(mouse_report_type));
					}
				}
				if constexpr (keyboard_included_type::value) {
					if (IS(cmd.pendingFlush, KEYBOARD)) {
						if (ANY_EXCEPT(cmd.pendingFlush, KEYBOARD)) {
							//has any except keyboard itself
							//apply small delay to improve combination reading from OS
							waitMs(generateDelay(SHORT_SPACER));
						}
						waitTransferCompletion();
						const keyboard_report_type emptyKb = {};
						tud_hid_report(HID_ITF_PROTOCOL_KEYBOARD, &emptyKb, sizeof(keyboard_report_type));
					}
				}
			}

			void pushAndRelease(const command& cmd, const uint32_t delay) {
				flushPush(cmd);
				waitMs(delay);
				flushRelease(cmd);
			}

			inline bool lockIfUnset(std::mutex& mutex, int& check, const int mask) const {
				if (!IS(check, mask)) {
					/*debug("perform lock", mask);*/
					mutex.lock(); SET(check, mask);
					return true;
				}
				return false;
			}

			inline bool unlockIfSet(std::mutex& mutex, int& check, const int mask) const {
				if (IS(check, mask)) {
					mutex.unlock(); UNSET(check, mask);
					return true;
				}
				return false;
			}

			void releaseDevLocks(local_context_type& ctx) {
				if constexpr (joystick0_included_type::value) {
					if (IS(ctx.lockedDev, JOYSTICK0)) {
						/*debug("release joy0");*/
						state.joystick0.mutex.unlock();
					}
				}
				if constexpr (joystick1_included_type::value) {
					if (IS(ctx.lockedDev, JOYSTICK1)) {
						/*debug("release joy1");*/
						state.joystick1.mutex.unlock();
					}
				}
				if constexpr (mouse_included_type::value) {
					if (IS(ctx.lockedDev, MOUSE)) {
						/*debug("release mouse");*/
						state.mouse.mutex.unlock();
					}
				}
				ctx.lockedDev = 0;
			}

			inline void throttleMs(const uint32_t throttle) {
				if (const auto left = (esp_timer_get_time() - lastKeyPressUS) / 1000; left < throttle) {
					debugIf(LOG_USB_DEVIVE, "throttleMs", throttle-left);
					waitMs(throttle-left);
				}
			}

			inline bool backendReady() const {
				/**
				 * small debug workaround if device not mount it's ready
				 * it will allow to retrive debug information if OTG not connected
				 */
				return !tud_mounted() || tud_hid_ready();
			}

			inline void waitTransferCompletion() const {
				//we must yield as tud task may be on same core as we are
				while (!backendReady()) { taskYIELD(); };
			}

			void waitMs(const uint32_t ms) {
				int64_t startTime = esp_timer_get_time();
				int64_t us 		  = std::max((int64_t)ms * 1000 - 500, (int64_t)0);
				tasking::task::this_sleep_for(ms);
				//now we are in 1hz range of target time, by default it's 10ms
				int64_t startYeld = esp_timer_get_time();
				while (esp_timer_get_time() - startTime < us) { taskYIELD(); }
				//now we are before time ~500us or after target time with some dT
				debugIf(LOG_USB_DEVIVE, "waitMs us:", ms * 1000, " actual: ", esp_timer_get_time() - startTime, " yelding: ", esp_timer_get_time() - startYeld);
			}

			template<typename... MUTEX>
			auto lockForDirectAccess(const bool nowait, const bool nolock, MUTEX&... m) {
				if (nolock) { /*debug("lockForDirectAccess", "nolock");*/ return  syncing::scoped_lock(std::defer_lock, m...); }
				/*debug("lockForDirectAccess", nowait ? (backendReady() ? "trylock" : "defer") : "lock");*/
				return nowait ? (
					backendReady() ?
						syncing::scoped_lock(std::try_to_lock, m...) :
						syncing::scoped_lock(std::defer_lock, m...)
				) : syncing::scoped_lock(m...);
			}

			int collect(const reportUnpacker::result_type& shard, command& cmd, local_context_type& ctx) {
				switch (shard.type) {
				case KB_BUTTONS:
					if constexpr (keyboard_included_type::value) {
						memcpy(
							&cmd.keyboard.keycode,
							shard.report->keyboard_buttons.data,
							sizeof(keyboard_report_type::keycode)
						);
						cmd.keyboard.modifier = shard.report->keyboard_buttons.modifier;
						cmd.keyboard.reserved = REPORT_UNMASK(shard.report->keyboard_buttons.flags);
						SET(cmd.pendingFlush,  KEYBOARD);
					}
					break;
				case JS0_AXIS0:
				case JS0_AXIS1:
				case JS0_AXIS2:
				case JS0_AXIS3:
				case JS0_AXIS4:
				case JS0_AXIS5:
				case JS0_AXIS6:
				case JS0_AXIS7:
					if constexpr (joystick0_included_type::value) {
						/*debug("jo0 axi data", shard.type-JS0_AXIS0);*/
						lockIfUnset(state.joystick0.mutex, ctx.lockedDev, JOYSTICK0);
						( (int16_t*)&state.joystick0.unsafe )[shard.type-JS0_AXIS0] = shard.report->joystick_axis;
						SET(cmd.pendingFlush,  JOYSTICK0);
					}
					break;
				case JS1_AXIS0:
				case JS1_AXIS1:
				case JS1_AXIS2:
				case JS1_AXIS3:
				case JS1_AXIS4:
				case JS1_AXIS5:
				case JS1_AXIS6:
				case JS1_AXIS7:
					if constexpr (joystick1_included_type::value) {
						/*debug("jo1axi data", shard.type-JS1_AXIS0);*/
						lockIfUnset(state.joystick1.mutex, ctx.lockedDev, JOYSTICK1);
						( (int16_t*)&state.joystick1.unsafe )[shard.type-JS1_AXIS0] = shard.report->joystick_axis;
						SET(cmd.pendingFlush, JOYSTICK1);
					}
					break;
				case JS0_BUTTONS:
					if constexpr (joystick0_included_type::value) {
						cmd.joystick0.buttons = shard.report->joystick_buttons.buttons;
						cmd.joystick0.flags = shard.report->joystick_buttons.flags;
						SET(cmd.pendingFlush, JOYSTICK0);
					}
					break;
				case JS1_BUTTONS:
					if constexpr (joystick1_included_type::value) {
						cmd.joystick1.buttons = shard.report->joystick_buttons.buttons;
						cmd.joystick1.flags = shard.report->joystick_buttons.flags;
						SET(cmd.pendingFlush, JOYSTICK1);
					}
					break;
				case MS_AXES:
					if constexpr (mouse_included_type::value) {
						lockIfUnset(state.mouse.mutex, ctx.lockedDev, MOUSE);
						if (
							state.mouse.unsafe.x == shard.report->mouse_axis[0] &&
							state.mouse.unsafe.y == shard.report->mouse_axis[1]
						) {
							//OS cache passthrough
							state.mouse.unsafe.x = (int16_t)(shard.report->mouse_axis[0] + 1);
							state.mouse.unsafe.y = (int16_t)(shard.report->mouse_axis[1] + 1);
						} else {
							state.mouse.unsafe.x = shard.report->mouse_axis[0];
							state.mouse.unsafe.y = shard.report->mouse_axis[1];
						}
						SET(cmd.pendingFlush, MOUSE);
					}
					break;
				case MS_SCROLLS:
					if constexpr (mouse_included_type::value) {
						cmd.mouse.pan = shard.report->mouse_scroll[0];
						cmd.mouse.wheel = -shard.report->mouse_scroll[1];
						SET(cmd.pendingFlush, MOUSE);
					}
					break;
				case MS_BUTTONS:
					if constexpr (mouse_included_type::value) {
						cmd.mouse.buttons = shard.report->mouse_buttons.buttons;
						cmd.mouse.flags = shard.report->mouse_buttons.flags;
						SET(cmd.pendingFlush, MOUSE);
					}
					break;
				default:
					return ESP_FAIL;
				}
				return ESP_OK;
			}

			static void evaluateCommandFlags(command& cmd) {
				cmd.flags = 0x00;
				if (keyboard_included_type::value && IS(cmd.pendingFlush, KEYBOARD)) {
					cmd.flags = cmd.keyboard.reserved;
				} else if  (joystick0_included_type::value && IS(cmd.pendingFlush, JOYSTICK0)) {
					cmd.flags = cmd.joystick0.flags;
				} else if (joystick1_included_type::value && IS(cmd.pendingFlush, JOYSTICK1)) {
					cmd.flags = cmd.joystick1.flags;
				} else if (mouse_included_type::value && IS(cmd.pendingFlush, MOUSE)) {
					cmd.flags = cmd.mouse.flags;
				}
			}

			[[noreturn]] static void loop(void* arg) {
				//if device not mounted wait for it once for 1s
				if (!tud_mounted()) { ::tasking::task::this_sleep_for(1000); }
				while (true) { (*static_cast<compositeTask*>(arg))(); }
			}

		public:

			typedef std::function<float()> entropy_generator_type;
			typedef result<uint32_t> push_result;
			typedef guardianRestrictor<joystick_direct_type, syncing::scoped_lock<std::mutex, std::mutex>> joystick_direct_guardian_type;
			typedef guardianRestrictor<mouse_direct_type, syncing::scoped_lock<std::mutex, std::mutex>> mouse_direct_guardian_type;
			typedef uint16_t AXIS_TYPE;

			using keyboard_included_type = std::bool_constant<INCLUDE_KEYBOARD>;
			using joystick0_included_type = std::bool_constant<INCLUDE_JOYSTICK0>;
			using joystick1_included_type = std::bool_constant<INCLUDE_JOYSTICK1>;
			using mouse_included_type = std::bool_constant<INCLUDE_MOUSE>;

			static constexpr auto MAX_DANGLING_REPORTS = 8;
			static constexpr AXIS_TYPE AXIS_MIN 	= 0;
			static constexpr AXIS_TYPE AXIS_MAX 	= 2048;
			static constexpr AXIS_TYPE AXIS_MIDDLE 	= (AXIS_MAX - AXIS_MIN) / 2 + AXIS_MIN;

			entropy_generator_type entropy = ([]() -> float { return 0.0; });

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

			compositeTask(): queue(25), task(&loop, "keyboardTask", 3072, this, 10) { };

			~compositeTask() = default;

			compositeTask(compositeTask&) = delete;

			compositeTask& operator=(compositeTask&) = delete;

			joystick_direct_guardian_type directJoystick(const bool nowait = true, const uint8_t joystickId = 0) {
				assert(joystickId == 0 || joystickId == 1);
				auto& js = joystickId == 0 ? state.joystick0 : state.joystick1;
				const bool wasDisabled = joystickId == 0 ? !joystick0_included_type::value : !joystick1_included_type::value;
				const int reportId = wasDisabled ? -1 : (joystickId == 0 ? REPORT_ID_GAMEPAD0 : REPORT_ID_GAMEPAD1);
				return {
					(joystick_direct_type*)&js.unsafe,
					lockForDirectAccess(nowait, wasDisabled,  js.mutex, flushMutex),
					[&, reportId] {
						//SBO probably for 3 int, we use only 2
						if constexpr (!DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC) {
							if (reportId != -1) {
								tud_hid_report(reportId, &js.unsafe, sizeof(joystick_report_type));
							}
						}
					}
				};
			}

			mouse_direct_guardian_type directMouse(const bool nowait = true) {
				return {
					(mouse_direct_type*)&state.mouse.unsafe.x,
					lockForDirectAccess(nowait, !mouse_included_type::value, state.mouse.mutex, flushMutex),
					[&] {
						if constexpr (mouse_included_type::value && !DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC) {
							tud_hid_report(HID_ITF_PROTOCOL_MOUSE, &state.mouse.unsafe, sizeof(mouse_report_type));
						}
					}
				};
			}

			inline void operator()() {

				report rcvBuffer;
				reportUnpacker unpacker(rcvBuffer);
				if(queue.pop_front(rcvBuffer, pdMS_TO_TICKS(30))) {
					reportUnpacker::result_type shard;
					local_context_type ctx{}; //some data that MUST be local
					do {
						while (collect(shard = unpacker.unpack_front(), current, ctx) == ESP_OK) {};
						if (current.pendingFlush) {
							evaluateCommandFlags(current);
							if (!executeCommand(current)) {
								//dumbCombination("unable executeCommand", current);
							}
							current = {};
						}
						releaseDevLocks(ctx); //must be after executeCommand, but in case if there is a bug call it anyway
						if (shard.type == TEXT) {
							auto ret = executeTyping(
								std::string_view(
									shard.report->text.contentNoNil,
									shard.report->text.size
								),
								shard.report->text.flags,
								current
							);
							if (!ret) {
								error("error while typing text", ret, " ", shard.report->text.flags);
							}
							current = {};
						}
					} while (shard.type != EMPTY);
					processedPacketCounter++;
				} else {
					//at this point there is no other commands (except direct)
					//for at least 30ms
					if (hasAnyDangling && !usingDanglingKeys) {
						flushPush(getMergedDanglingCommand(danglingCommands));
						lastKeyPressUS = esp_timer_get_time();
						usingDanglingKeys = true;
					}
				}
			}
								
			push_result push_back(const report& report) {
				return queue.push_back(report, 0);
			}
			
			[[nodiscard]] inline uint32_t receivedCnt() const noexcept {
				return receivedPacketCounter;
			} 
			
			[[nodiscard]] inline uint32_t processedCnt() const noexcept {
				return processedPacketCounter;
			} 
		
	};
	
}