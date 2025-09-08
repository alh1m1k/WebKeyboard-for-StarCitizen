#pragma once

#include <cstring>
#include <sys/_stdint.h>
#include "tinyusb.h"
#include <mutex>

#include "generated.h"

#include "usbDevice.h"
#include "deviceDescriptor.h"

#ifndef LOG_JOYSTICK 
#define LOG_JOYSTICK true
#endif

namespace hid {
			
	class joystick: public virtual usbModule {
		
		std::mutex m = {};
		
		uint32_t commandCounter = 0;
		
		bool _installed = false;
		
		struct TU_ATTR_PACKED state_s
		{
			uint16_t x;
			uint16_t y;
			uint16_t rx;
			uint16_t ry;
			uint16_t ls;
			uint16_t rs;
			uint16_t ld;
			uint16_t rd;
			uint32_t buttons;
		} state;
		
		class controlWriter {
		
			joystick& owner;
			std::unique_lock<std::mutex> guardian;
			
			
			public:
									
				controlWriter(joystick& owner, std::unique_lock<std::mutex>&& guardian): owner(owner), guardian(std::move(guardian)) {
	
				}
				
				~controlWriter() {
					owner.writeAxis();
				}
				
				controlWriter(controlWriter&& other) = default;
											
				operator bool() {
					return true;
				}
				
				//todo change me to method modifiers
				state_s* operator ->() {
					return &owner.state;
				}
				
				size_t copyFrom(const void* ptr, size_t size) {
					size = std::min(size, sizeof(state));
					memcpy(&owner.state, ptr, size);
					return size;
				}
				
				size_t copyTo(void* ptr, size_t size) {
					size = std::min(size, sizeof(state));
					memcpy(ptr, &owner.state, size);
					return size;
				}
		};
		
		protected:
		
			bool writeAxis(const int axisId, uint16_t value);
			
			void writeAxis();
		
		public:
				
			typedef controlWriter  control_writer_type;		
				
			enum class axis {
				X = 1,
				Y,
				RX, 
				RY,
				Z,
				RZ,
				THROTTLE,
				CUSTOM_I
			} ;
			
			enum class ctrlAxis {
				PITCH,
				ROLL,
				YAW,
				THROTTLE = (int)axis::THROTTLE,
				CUSTOM_I = (int)axis::CUSTOM_I,
			};
			
			typedef uint16_t AXIS_TYPE;
			static const AXIS_TYPE AXIS_MIN 	= 0;
			static const AXIS_TYPE AXIS_MAX 	= 2048;
			static const AXIS_TYPE AXIS_MIDDLE 	= (AXIS_MAX - AXIS_MIN) / 2 + AXIS_MIN;
		
			joystick();
						
			~joystick();
			
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
				return commandCounter;
			}
			
			inline uint32_t processedCnt() {
				return commandCounter;
			} 
			
			inline bool axis(const int axisId, uint16_t value) {
				auto guardian = std::unique_lock(m);
				return writeAxis(axisId, value);
			}
			
			control_writer_type control() {
				return {*this, std::unique_lock(m)};
			}
			
			bool custom(const int axisId, uint16_t value) {
				if (isControlAxis(axisId)) {
					return false;
				}
				auto guardian = std::unique_lock(m);
				return writeAxis(axisId, value);
			}
			
			inline bool isControlAxis(const int axisId) {
				if (axisId >= 0 && axisId <= (int)axis::THROTTLE) {
					return true;
				}
				return false;
			}
						
			bool 			setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) override;

			uint16_t 		getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) override;
	};
	
}

