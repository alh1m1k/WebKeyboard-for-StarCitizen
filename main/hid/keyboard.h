#pragma once 

#include <memory>
#include <mutex>
#include <functional>
#include <sys/unistd.h>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/_stdint.h>

#include "generated.h"

#include "sequenceWriter.h"
#include "tinyusb.h"

#include "keyboardTask.h"
#include "writer.h"

#include "usbDevice.h"

#ifndef LOG_KEYBOARD 
#define LOG_KEYBOARD true
#endif

namespace hid {
			
	class keyboard: public writer<std::unique_ptr<keyboardTask>>, public virtual usbModule {
		
		typedef std::unique_ptr<keyboardTask> 	task_type;
		typedef std::unique_lock<std::mutex> 	combine_guardian_type;
				
		bool _installed = false;
		
		std::mutex m = {};
		
		task_type 										task  = nullptr;
				
		uint8_t leds = 0;
		
		std::function<void(uint8_t ledStatus, uint8_t ledStatusPrev)> ledStatusChangeCallback = {};
												
		protected:
		
			bool write(uint8_t charCode, uint8_t modifier, const uint8_t type) {
								
				auto locker = std::unique_lock(m);
				
				debugIf(LOG_KEYBOARD, "keyboard::write::type", (int)type);

				return writer<task_type>::write(charCode, modifier, type);
			}

				
		public:
				
			typedef writer<task_type> 												writer_type;
			typedef sequenceWriter<task_type, std::mutex> 							sequence_writer_type;
			typedef std::function<void(uint8_t ledStatus, uint8_t ledStatusPrev)> 	onLedStatusChangeCallback;
			
			using writer_type::symbol;
			using writer_type::special;
			
			using modifier = hid_keyboard_modifier_bm_t;
								
			keyboard();
						
			~keyboard();
			
			bool install();
						
			void deinstall();
			
			inline bool installed() const noexcept {
				return _installed;
			}
			
			bool mounted() const noexcept;
			
			inline operator bool() const noexcept {
				return installed() && mounted();
			}
			
			inline uint32_t receivedCnt() {
				return task->receivedCnt();
			}
			
			inline uint32_t processedCnt() {
				return task->processedCnt();
			} 
			
			inline uint8_t ledStatus() {
				return leds;
			} 
			
			//warning chack for perfomance of callback
			//todo mb use default EventLoop for this
			inline void onLedStatusChange(const onLedStatusChangeCallback& callback) {
				ledStatusChangeCallback = callback;
			} 
		
			/*
			* must be call after install or it be null deref
			*/
			template<class source>
			void entroySource(source generator) {
				task->entropy = generator;
			}
			
			sequence_writer_type sequence() {
				return {task, std::unique_lock(m)};
			}
			
			void sequence(std::vector<uint8_t> keys) {
				
			}
			
			bool 			setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) override;

			uint16_t 		getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) override;
			
	};
	
}