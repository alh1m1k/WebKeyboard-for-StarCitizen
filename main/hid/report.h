#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace hid {

	struct report {

		union {
			uint8_t storage[8];
			struct {
				uint8_t	header[4];
				void* advancedPtr;
			};
		};

		struct keyboard_buttons_type { //must be exact 8 bytes
			uint8_t flags 		= 0; //must be first to mimic type, this is different from native
			uint8_t modifier 	= 0;
			uint8_t data[6] 	= {0, 0, 0, 0, 0, 0};
		};

		//this is pointer type only
		struct __attribute__ ((packed)) text_type {
			uint8_t flags;
			uint16_t size;
			char contentNoNil[std::numeric_limits<uint16_t>::max()];
		};

		typedef int16_t	joystick_axis_type;
		struct __attribute__ ((packed)) joystick_buttons_type {
			uint8_t flags;
			uint32_t buttons;
		};

		struct mouse_buttons_type {
			uint8_t buttons;
			uint8_t flags;
		};
		typedef int8_t mouse_scrolls_type[2];
		typedef int16_t mouse_axes_type[2];

		enum PacketType: uint8_t {
			EMPTY = 0,
			EMPTY_HEADER, // for keyboard_buttons_type this reserved(special meaning) allow not to place `type` before first keyboard_buttons_type
			TEXT,
			KB_BUTTONS,
			JS0_AXIS0,
			JS0_AXIS1,
			JS0_AXIS2,
			JS0_AXIS3,
			JS0_AXIS4,
			JS0_AXIS5,
			JS0_AXIS6,
			JS0_AXIS7,
			JS0_BUTTONS,
			JS1_AXIS0,
			JS1_AXIS1,
			JS1_AXIS2,
			JS1_AXIS3,
			JS1_AXIS4,
			JS1_AXIS5,
			JS1_AXIS6,
			JS1_AXIS7,
			JS1_BUTTONS,
			MS_AXES,
			MS_SCROLLS,
			MS_BUTTONS,
			TOTAL
		};

		report() noexcept : storage(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) { };

		report(const report&) = default;
		report(report&& move) = default;
		~report() = default;
		report& operator=(const report&) = default;
		report& operator=(report&& move) = default;

	};

}