#pragma once


#include "generated.h"

#include <stdexcept>

#include "asciiDictionary.h"
#include "class/hid/hid.h"
#include "compositeWriter.h"
#include "esp_err.h"
#include "reportPacker.h"
#include "result.h"
#include "util.h"

namespace hid {

	template<class BACK_PUSHER>
	class compositeCombinationWriter {

		using base_write_result_type = compositeWriter<BACK_PUSHER>::write_result_type;
		using enum report::PacketType;

		protected:

			BACK_PUSHER& backend;
			base_write_result_type flushResult = (esp_err_t)errors::INCOMPLETE;
			pressType pt;

			typedef std::remove_extent_t<report::mouse_axes_type> mouse_axes_axis_type;
			static constexpr auto mouse_axes_axis_count = std::extent<report::mouse_axes_type>::value;

			typedef std::remove_extent_t<report::mouse_scrolls_type> mouse_scroll_axis_type;
			static constexpr auto mouse_scroll_axis_count = std::extent<report::mouse_scrolls_type>::value;

			std::vector< //expect that vector<pair> was faster unordered_map<> if size < 25
				std::pair<
					uint8_t,
					std::variant<
						std::unique_ptr<report::keyboard_buttons_type>,
						std::unique_ptr<report::joystick_axis_type>,
						std::unique_ptr<report::joystick_buttons_type>,
						std::unique_ptr<mouse_axes_axis_type[]>, //to safely allocate memory, with std::make_unique (static size array forbidden to make_unique)
						std::unique_ptr<mouse_scroll_axis_type[]>, //to safely allocate memory, with std::make_unique (static size array forbidden to make_unique)
						std::unique_ptr<report::mouse_buttons_type>
					>
				>
			> buffer = {};

			template<typename T, bool MakeItFirst = false>
			inline auto& refCtr(const uint8_t type){
				static_assert(!std::is_array_v<T>, "T must be non array type");
				for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
					if (it->first == type) {
						return std::get<std::unique_ptr<T>>(it->second);
					}
				}
				if constexpr (MakeItFirst) {
					std::swap(buffer.emplace_back(type, std::make_unique<T>()), buffer[0]);
					return std::get<std::unique_ptr<T>>(buffer[0].second);
				} else {
					return std::get<std::unique_ptr<T>>(
						buffer.emplace_back(type, std::make_unique<T>()).second
					);
				}
			}

			template<typename T> //array allocator version
			inline auto& refCtr(const uint8_t type, const size_t size){
				static_assert(std::is_array_v<T>, "T must be array type");
				for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
					if (it->first == type) {
						return std::get<std::unique_ptr<T>>(it->second);
					}
				}
				return std::get<std::unique_ptr<T>>(
					buffer.emplace_back(type, std::make_unique<T>(size)).second
				);
			}

			template<typename T>
			inline auto findKBSlot(T& array) {
				static_assert(std::is_array_v<T>, "must be array type");
				for (auto i = 0; i < std::extent<T>::value; ++i) {
					if (array[i] == 0x00) { return i; }
				}
				return -1;
			}

			void writeKeyboardSymbol(const char symbol) {
				const uint8_t keyCode = ASCII2KEYCODE[(uint8_t)symbol][1];
				writeKeyboardButtons(keyCode);
			}

			void writeKeyboardButtons(const uint8_t keyCode) {
				const auto& ctrl = refCtr<report::keyboard_buttons_type, true>(KB_BUTTONS);
				if (const auto mdf = extractModifier(keyCode); mdf != 0) {
					ctrl->modifier |= mdf;
				} else {
					if (auto slot = findKBSlot(ctrl->data); slot != -1) {
						ctrl->data[slot] = keyCode;
					} else {
						throw std::out_of_range("keyboard supports only 6 keys in combination");
					}
				}
			}

			void writeJoystickAxis(const uint8_t axisId, const int16_t value, const uint8_t joystickId = 0) {
				const uint16_t reportType = JS0_AXIS0 + axisId + (joystickId * 9);
				assert((reportType >= JS0_AXIS0 && reportType < JS0_BUTTONS) || (reportType >= JS1_AXIS0 && reportType < JS1_BUTTONS));
				const auto& ctrl = refCtr<report::joystick_axis_type>(reportType);
				*ctrl = value;
			}

			void writeJoystickButtons(const uint32_t btnBitmap, const uint8_t joystickId = 0) {
				const uint16_t reportType = JS0_BUTTONS + (joystickId * 9);
				assert(reportType == JS0_BUTTONS || reportType == JS1_BUTTONS);
				const auto& ctrl = refCtr<report::joystick_buttons_type>(reportType);
				ctrl->buttons |= btnBitmap;
			}

			void writeMousePosition(const int16_t posX, const int16_t posY) {
				const auto& ctrl = refCtr<mouse_axes_axis_type[]>(MS_AXES, 2);
				ctrl[0] = posX;
				ctrl[1] = posY;
			}

			void writeMouseScroll(const int8_t posX, const int8_t posY) {
				const auto& ctrl = refCtr<mouse_scroll_axis_type[]>(MS_SCROLLS, 2);
				ctrl[0] = posX;
				ctrl[1] = posY;
			}

			void writeMouseButtons(uint8_t btnBitmap) {
				const auto& ctrl = refCtr<report::mouse_buttons_type>(MS_BUTTONS);
				ctrl->buttons |= btnBitmap;
			}
									
			uint8_t extractModifier(const uint8_t charCode) {
				switch (charCode) {
					case HID_KEY_SHIFT_LEFT:    
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTSHIFT;              
					case HID_KEY_ALT_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTALT;                  
					case HID_KEY_GUI_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTGUI;    
					case HID_KEY_CONTROL_LEFT:
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_LEFTCTRL;                 
					case HID_KEY_CONTROL_RIGHT:   
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTCTRL;            
					case HID_KEY_SHIFT_RIGHT:  
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTSHIFT;                
					case HID_KEY_ALT_RIGHT: 
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTALT;                 
					case HID_KEY_GUI_RIGHT: 
						return hid_keyboard_modifier_bm_t::KEYBOARD_MODIFIER_RIGHTGUI;
				}
				return 0;
			}

			void flush() {

				reportPacker packer = {};
				for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
					//none: we not able to allocate memory inside switch
					switch (it->first) {
					case KB_BUTTONS:
						std::get<std::unique_ptr<report::keyboard_buttons_type>>(it->second)->flags = (uint8_t)pt;
						packer.pack_back(
							it->first,
							*std::get<std::unique_ptr<report::keyboard_buttons_type>>(it->second)
						);
						break;
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
						packer.pack_back(
							it->first,
							*std::get<std::unique_ptr<report::joystick_axis_type>>(it->second)
						);
						break;
					case JS0_BUTTONS:
					case JS1_BUTTONS:
						std::get<std::unique_ptr<report::joystick_buttons_type>>(it->second)->flags = (uint8_t)pt;
						packer.pack_back(
							it->first,
							*std::get<std::unique_ptr<report::joystick_buttons_type>>(it->second)
						);
						break;
					case MS_AXES:
						packer.pack_back(
							it->first,
							*(mouse_axes_axis_type (*)[mouse_axes_axis_count])std::get<std::unique_ptr<mouse_axes_axis_type[]>>(it->second).get()
						);
						break;
					case MS_SCROLLS:
						packer.pack_back(
							it->first,
							*(mouse_scroll_axis_type (*)[mouse_scroll_axis_count])std::get<std::unique_ptr<mouse_scroll_axis_type[]>>(it->second).get()
						);
						break;
					case MS_BUTTONS:
						std::get<std::unique_ptr<report::mouse_buttons_type>>(it->second)->flags = (uint8_t)pt;
						packer.pack_back(
							it->first,
							*std::get<std::unique_ptr<report::mouse_buttons_type>>(it->second)
						);
						break;
					default:
						error("unsupported packet", it->first);
					}
				}
				buffer.clear();

				report report = {};
				packer.repack(report);
				flushResult = backend->push_back(report);
			}
		
		public:

			enum class errors: esp_err_t {
				INCOMPLETE = -100000,
				OVERFLOW,
			};

			using write_result_type = base_write_result_type;

			explicit compositeCombinationWriter(
				BACK_PUSHER& backend,
				const pressType press = pressType::PRESS,
				const uint8_t modifier = 0,
				write_result_type* const outResult = nullptr
			) : backend(backend), pt(press)
			{
				if (modifier != 0) {
					keyboardModifier(modifier);
				}
			}
			
			~compositeCombinationWriter() {
				if (buffer.size()) { flush(); }
			}
			
			auto& modify(const uint8_t mdf) {
				return keyboardModifier(mdf);
			}
						
			auto& symbol(const char symbol) {
				return keyboardSymbol(symbol);
			}
		
			auto& special(const uint8_t charCode) {
				return keyboardKey(charCode);
			}

			auto& keyboardModifier(const int code) {
				refCtr<report::keyboard_buttons_type, true>(KB_BUTTONS)->modifier |= code;
				return *this;
			}

			auto& keyboardSymbol(const char symbol) {
				writeKeyboardSymbol(symbol);
				return *this;
			}

			auto& keyboardKey(const uint8_t keyCode) {
				writeKeyboardButtons(keyCode);
				return *this;
			}

			auto& joystickAxis(const uint8_t axisId, const int16_t value, const uint8_t joystickId = 0) {
				writeJoystickAxis(axisId, value, joystickId);
				return *this;
			}

			auto& joystickButtons(const uint32_t btnBitmap, const uint8_t joystickId = 0) {
				writeJoystickButtons(btnBitmap, joystickId);
				return *this;
			}

			auto& mousePosition(const int16_t posX, const int16_t posY) {
				writeMousePosition(posX, posY);
				return *this;
			}

			auto& mouseScroll(const int8_t scrollX, const int8_t scrollY) {
				writeMouseScroll(scrollX, scrollY);
				return *this;
			}

			auto& mouseButtons(const uint8_t btnBitmap) {
				writeMouseButtons(btnBitmap);
				return *this;
			}
			
			auto& done() {
				flush();
				return *this;
			}
			
			operator bool() const noexcept {
				return !!backend;
			}
			
			write_result_type lastWriteResult() const noexcept {
				return flushResult;
			}
		
	};

}