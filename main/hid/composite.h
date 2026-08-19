#pragma once 

#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <sys/_stdint.h>

#include "generated.h"

#include "compositeCombinationWriter.h"
#include "compositeSequenceWriter.h"
#include "tinyusb.h"

#include "compositeTask.h"
#include "compositeWriter.h"

#include "not_implemented.h"
#include "usbDevice.h"

namespace hid {
			
	class composite: public compositeWriter<std::unique_ptr<compositeTask>>, public virtual usbModule {
		
		typedef std::unique_ptr<compositeTask> 	task_type;
		typedef std::unique_lock<std::mutex> 	combine_guardian_type;
				
		bool _installed = false;
		
		std::mutex m = {};
		
		task_type task  = nullptr;
				
		uint8_t leds = 0;
		
		std::function<void(uint8_t ledStatus, uint8_t ledStatusPrev)> ledStatusChangeCallback = {};

		public:
				
			using writer_type = compositeWriter<task_type>;
			using sequence_writer_type = compositeSequenceWriter<task_type, std::mutex>;
			using combination_writer_type = sequence_writer_type::combination_writer_type;
			using joystick_direct_guardian_type = compositeTask::joystick_direct_guardian_type;
			using mouse_direct_guardian_type = compositeTask::mouse_direct_guardian_type;
			typedef std::function<void(uint8_t ledStatus, uint8_t ledStatusPrev)> onLedStatusChangeCallback;

			using keyboard_included_type = compositeTask::keyboard_included_type;
			using joystick0_included_type = compositeTask::joystick0_included_type;
			using joystick1_included_type = compositeTask::joystick1_included_type;
			using mouse_included_type = compositeTask::mouse_included_type;

			using writer_type::keyboardSymbol;
			using writer_type::keyboardKey;

			
			using modifier = hid_keyboard_modifier_bm_t;
								
			composite();
						
			~composite();
			
			bool install();
						
			void deinstall();
			
			[[nodiscard]] bool installed() const noexcept {
				return _installed;
			}
			
			[[nodiscard]] bool mounted() const noexcept;
			
			inline operator bool() const noexcept {
				return installed() && mounted();
			}
			
			[[nodiscard]] inline uint32_t receivedCnt() const noexcept {
				return task->receivedCnt();
			}
			
			[[nodiscard]] inline uint32_t processedCnt() const noexcept {
				return task->processedCnt();
			} 
			
			[[nodiscard]] inline uint8_t ledStatus() const noexcept {
				return leds;
			} 
			
			//warning chack for performance of callback
			//todo mb use default EventLoop for this
			inline void onLedStatusChange(const onLedStatusChangeCallback& callback) {
				ledStatusChangeCallback = callback;
			} 
		
			/*
			* must be call after install or it be null deref
			*/
			template<class source>
			void entropySource(source generator) {
				task->entropy = generator;
			}
			
			sequence_writer_type sequence() {
				return {task, std::unique_lock(m)};
			}
			
			void sequence(const std::vector<uint8_t>& keys) {
				throw not_implemented("sequence");
			}

			combination_writer_type combination(const pressType press = pressType::PRESS, const uint8_t modifiers = 0) {
				if ((uint8_t)press == 0) {
					error("sequenceWriter::combination invalid press type", (uint8_t)press, " ", (uint8_t)modifiers);
				}
				return compositeCombinationWriter(backend, press, modifiers);
			}

			joystick_direct_guardian_type directJoystick(const bool nowait = true, const uint8_t joystickId = 0) {
				return task->directJoystick(nowait, joystickId);
			}

			mouse_direct_guardian_type directMouse(const bool nowait = true) {
				return task->directMouse(nowait);
			}
			
			bool setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) override;

			uint16_t getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) override;
			
	};
	
}