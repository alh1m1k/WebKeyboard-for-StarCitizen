#pragma once


#include "class/hid/hid_device.h"
#include "result.h"

#include "asciiDictionary.h"
#include "pressType.h"
#include "report.h"
#include "reportPacker.h"
#include "util.h"



namespace hid {
	
	template<class BACK_PUSHER>
	class compositeWriter {

		using enum report::PacketType;
		
		typedef result<uint32_t> base_write_result_type;

		static report makeReport(const uint8_t type, const report::keyboard_buttons_type& source) noexcept {
			report r;
			memcpy(r.storage, &source, sizeof(report::keyboard_buttons_type));
			r.storage[0] = REPORT_MASK(r.storage[0]);
			return r;
		}

		template <typename T>
		static report makeReport(const uint8_t type, const T& source) noexcept {
			static_assert(sizeof(T) + 1 <= sizeof(report::storage), "incorrect size of source");
			static_assert(std::is_trivially_copyable_v<T>, "source must be trivially copyable");
			report r;
			memcpy(&r.storage[1], &source, sizeof(T));
			r.storage[0] = type;
			return r;
		}

		protected:
		
			BACK_PUSHER& backend;

			uint8_t lastModifier = 0; 

			bool writeKeyboardSymbol(const char symbol, uint8_t modifier, const uint8_t type) {

				if (ASCII2KEYCODE[(uint8_t)symbol][0]) {
					modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
				}

				const uint8_t charCode = ASCII2KEYCODE[(uint8_t)symbol][1];

				return writeKeyboardButtons(charCode, modifier, type);
			}

			base_write_result_type writeKeyboardButtons(const uint8_t keyCode, const uint8_t modifier, const uint8_t type) {
				if (keyCode == 0) {
					error("shortcutWriter::write check flow value is zero");
				}
				return backend->push_back(
					makeReport(KB_BUTTONS, report::keyboard_buttons_type{type, modifier, {keyCode}})
				);
			}

			base_write_result_type writeJoystickAxis(const uint8_t axisId, const int16_t value, const uint8_t joystickId = 0) {
				const uint16_t reportType = JS0_AXIS0 + axisId + (joystickId * 9);
				assert((reportType >= JS0_AXIS0 && reportType < JS0_BUTTONS) || (reportType >= JS1_AXIS0 && reportType < JS1_BUTTONS));
				return backend->push_back(
					makeReport(reportType, (report::joystick_axis_type)value)
				);
			}

			base_write_result_type writeJoystickButtons(const uint32_t btnBitmap, const uint8_t type, const uint8_t joystickId = 0) {
				const uint16_t reportType = JS0_BUTTONS + (joystickId * 9);
				assert(reportType == JS0_BUTTONS || reportType == JS1_BUTTONS);
				return backend->push_back(
					makeReport(reportType, report::joystick_buttons_type{type, btnBitmap})
				);
			}

			base_write_result_type writeMousePosition(const int16_t posX, const int16_t posY) {
				return backend->push_back(
					makeReport(MS_AXES, report::mouse_axes_type{posX, posY})
				);
			}

			base_write_result_type writeMouseScroll(const int8_t posX, const int8_t posY) {
				return backend->push_back(
					makeReport(MS_SCROLLS, report::mouse_scrolls_type{posX, posY})
				);
			}

			base_write_result_type writeMouseButtons(const uint8_t btnBitmap, const uint8_t type) {
				return backend->push_back(
					makeReport(MS_BUTTONS, report::mouse_buttons_type{type, btnBitmap})
				);
			}

		public:

			using write_result_type = base_write_result_type;
			using press_type = pressType;
				
			compositeWriter(BACK_PUSHER& bp, const uint8_t modifier): backend(bp), lastModifier(modifier) {}

			auto& keyboardModifier(const int code) {
				lastModifier |= (int)code;
				return *this;
			}

			uint8_t& keyboardModifier() {
				return lastModifier;
			}

			write_result_type keyboardSymbol(const char symbol, const press_type press = press_type::PRESS) {
				return writeKeyboardSymbol(symbol, lastModifier, (uint8_t)press);
			}

			write_result_type keyboardKey(const uint8_t keyCode, const press_type press = press_type::PRESS) {
				return writeKeyboardButtons(keyCode, lastModifier, (uint8_t)press);
			}

			write_result_type joystickAxis(const uint8_t axisId, const int16_t value, const uint8_t joystickId = 0) {
				return writeJoystickAxis(axisId, value, joystickId);
			}

			write_result_type joystickButtons(const uint32_t btnBitmap, const press_type press = press_type::PRESS, const uint8_t joystickId = 0) {
				return writeJoystickButtons(btnBitmap, (uint8_t)press, joystickId);
			}

			write_result_type mousePosition(const int8_t posX, const int8_t posY) {
				return writeMousePosition(posX, posY);
			}

			write_result_type mouseScroll(const int8_t scrollX, const int8_t scrollY) {
				return writeMouseScroll(scrollX, scrollY);
			}

			write_result_type mouseButtons(const uint8_t btnBitmap, const press_type press = press_type::PRESS) {
				return writeMouseButtons(btnBitmap, (uint8_t)press);
			}

	};
}