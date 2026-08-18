#pragma once

#include "report.h"

#include "util.h"

namespace hid {

	using enum report::PacketType;

	static constexpr uint8_t KEYBOARD_MASKING_FLAG = 0x80;

	//static uint32_t alloces = 0;

	struct advanced_report {
		size_t size = 0x00;
		uint8_t* buffer = nullptr;
		advanced_report(const uint8_t * source, const size_t size): size(size), buffer(new uint8_t[size]) {
			memcpy(buffer, source, size);
			//debug("<--ALLOC-->", ++alloces);
		}
		advanced_report(
			const uint8_t * s1, const size_t s1sz,
			const uint8_t * s2, const size_t s2sz
		): size(s1sz + s2sz), buffer(new uint8_t[size]) {
			memcpy(buffer, s1, s1sz);
			memcpy(&buffer[s1sz], s2, s2sz);
			//debug("<--ALLOC-->", ++alloces);
		}
		~advanced_report() { delete[] buffer; /*debug("<--DE-ALLOC-->", --alloces);*/ }
	};

	static constexpr ssize_t sizeOfType(const uint8_t type) {
		switch (type) {
		case TEXT:
			return -1; //has dynamic size
		case KB_BUTTONS:
			return sizeof(report::keyboard_buttons_type);
		case JS0_AXIS0:
		case JS0_AXIS1:
		case JS0_AXIS2:
		case JS0_AXIS3:
		case JS0_AXIS4:
		case JS0_AXIS5:
		case JS0_AXIS6:
		case JS0_AXIS7:
		case JS1_AXIS0:
		case JS1_AXIS1:
		case JS1_AXIS2:
		case JS1_AXIS3:
		case JS1_AXIS4:
		case JS1_AXIS5:
		case JS1_AXIS6:
		case JS1_AXIS7:
			return sizeof(report::joystick_axis_type);
		case JS0_BUTTONS:
		case JS1_BUTTONS:
			return sizeof(report::joystick_buttons_type);
		case MS_AXES:
			return sizeof(report::mouse_axes_type);
		case MS_SCROLLS:
			return sizeof(report::mouse_scrolls_type);
		case MS_BUTTONS:
			return sizeof(report::mouse_buttons_type);
		default:
			if ((type&KEYBOARD_MASKING_FLAG) == KEYBOARD_MASKING_FLAG) { //this is buttons masking flag
				return sizeof(report::keyboard_buttons_type);
			}
			return 0;
		}
	}

	[[nodiscard]] static bool isAdvancedReport(const report& report) noexcept {
		if (report.storage[0] == EMPTY || report.storage[0] == EMPTY_HEADER) {
			return report.advancedPtr != nullptr;
		}
		if (sizeOfType(report.storage[0]) <= 3) { //it must fit inside 4bytes - 1 for type
			return report.advancedPtr != nullptr;
		} else {
			return false;
		}
	}

	static void recycle(report& report) noexcept {
		if (isAdvancedReport(report)) {
			report.storage[0] = TOTAL;
			delete (advanced_report*)report.advancedPtr;
			report.advancedPtr = nullptr;
		}
	}

}


