#include "keyboard.h"

namespace interpreter {

	using namespace std::literals;

	bool keyboard::specialKey(
		hid::composite::combination_writer_type& combination,
		const std::string_view& str,
		const word_type& tock
	) {

		/* ------------------------------------------------------------------------------------- */

		if (str == "alt"sv || str ==  "lalt"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_LEFTALT);
			return true;
		}

		if (str == "ralt"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_RIGHTALT);
			return true;
		}

		if (str == "ctrl"sv || str ==  "lctrl"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_LEFTCTRL);
			return true;
		}

		if (str == "rctrl"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_RIGHTCTRL);
			return true;
		}

		if (str == "shift"sv || str ==  "lshift"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_LEFTSHIFT);
			return true;
		}

		if (str == "rshift"sv) {
			combination.keyboardModifier(hid::composite::modifier::KEYBOARD_MODIFIER_RIGHTSHIFT);
			return true;
		}


		/* ------------------------------------------------------------------------------------- */
		/* Key variant of modifier key (workaround) */

		if (str == "alt-key"sv || str ==  "lalt-key"sv) {
			combination.keyboardKey(HID_KEY_ALT_LEFT);
			return true;
		}

		if (str == "ralt-key"sv) {
			combination.keyboardKey(HID_KEY_ALT_RIGHT);
			return true;
		}

		if (str == "ctrl-key"sv || str ==  "lctrl-key"sv) {
			combination.keyboardKey(HID_KEY_CONTROL_LEFT);
			return true;
		}

		if (str == "rctrl-key"sv) {
			combination.keyboardKey(HID_KEY_CONTROL_RIGHT);
			return true;
		}

		if (str == "shift-key"sv || str ==  "lshift-key"sv) {
			combination.keyboardKey(HID_KEY_SHIFT_LEFT);
			return true;
		}

		if (str == "rshift-key"sv) {
			combination.keyboardKey(HID_KEY_SHIFT_RIGHT);
			return true;
		}

		/* ------------------------------------------------------------------------------------- */
		/* Key variant of modifier key (workaround) */

		if (str == "keyboardKey-1"sv) {
			//if (auto combination = combination.combination(conv(tock.press)); combination) {
				debugIf(LOG_USB_DEVIVE, "keyboardKey-1 init");
				combination.keyboardKey(HID_KEY_ALT_LEFT);
				combination.keyboardKey('n');
				debugIf(LOG_USB_DEVIVE, "keyboardKey-1 n");
			//}
			debugIf(LOG_USB_DEVIVE, "keyboardKey-1 out of scope");
			return true;
		}

		if (str == "keyboardKey-2"sv) {
			//if (auto combination = combination.combination(conv(word_type::press_e::LONGPRESS)); combination) {
				debugIf(LOG_USB_DEVIVE, "keyboardKey-2 init");
				combination.keyboardKey(HID_KEY_ALT_LEFT);
				combination.keyboardKey(HID_KEY_CONTROL_LEFT);
				combination.keyboardKey(HID_KEY_DELETE);
				debugIf(LOG_USB_DEVIVE, "keyboardKey-2 n");
			//}
			debugIf(LOG_USB_DEVIVE, "keyboardKey-2 out of scope");
			return true;
		}

		if (str == "keyboardKey-3"sv) {
			//if (auto combination = combination.combination(conv(word_type::press_e::PRESS)); combination) {
				debugIf(LOG_USB_DEVIVE, "keyboardKey-3 init");
				combination.keyboardKey(HID_KEY_ALT_LEFT);
				combination.keyboardKey('a');
				combination.keyboardKey('b');
				combination.keyboardKey('c');
				debugIf(LOG_USB_DEVIVE, "keyboardKey-3 n");
			//}
			debugIf(LOG_USB_DEVIVE, "keyboardKey-3 out of scope");
			return true;
		}

		if (str == "keyboardKey-4"sv) {
			//if (auto combination = combination.combination(conv(word_type::press_e::DOUBLETAP)); combination) {
				debugIf(LOG_USB_DEVIVE, "keyboardKey-4 init");
				combination.keyboardKey(HID_KEY_ALT_LEFT);
				combination.keyboardKey('a');
				combination.keyboardKey('b');
				combination.keyboardKey('c');
				combination.keyboardKey('d');
				combination.keyboardKey('e');
				debugIf(LOG_USB_DEVIVE, "keyboardKey-4 n");
			//}
			debugIf(LOG_USB_DEVIVE, "keyboardKey-3 out of scope");
			return true;
		}


		/* ------------------------------------------------------------------------------------- */

		if (str == "scrolllock"sv) {
			combination.keyboardKey(HID_KEY_SCROLL_LOCK);
			return true;
		}

		if (str == "capslock"sv) {
			combination.keyboardKey(HID_KEY_CAPS_LOCK);
			return true;
		}

		if (str == "numlock"sv) {
			combination.keyboardKey(HID_KEY_NUM_LOCK);
			return true;
		}

		if (str == "numdiv"sv) {
			combination.keyboardKey(HID_KEY_KEYPAD_DIVIDE);
			return true;
		}

		if (str == "nummul"sv) {
			combination.keyboardKey(HID_KEY_KEYPAD_MULTIPLY);
			return true;
		}

		if (str == "numsub"sv) {
			combination.keyboardKey(HID_KEY_KEYPAD_SUBTRACT);
			return true;
		}

		if (str == "numadd"sv) {
			combination.keyboardKey(HID_KEY_KEYPAD_ADD);
			return true;
		}

		if (str == "numenter"sv) {
			combination.keyboardKey(HID_KEY_KEYPAD_ENTER);
			return true;
		}


		if (str == "numrigth"sv) {
			combination.keyboardKey(HID_KEY_ARROW_RIGHT);
			return true;
		}

		if (str == "numleft"sv) {
			combination.keyboardKey(HID_KEY_ARROW_LEFT);
			return true;
		}

		if (str == "numdown"sv) {
			combination.keyboardKey(HID_KEY_ARROW_DOWN);
			return true;
		}

		if (str == "numup"sv) {
			combination.keyboardKey(HID_KEY_ARROW_UP);
			return true;
		}

		if (str.size() == 4 && str.starts_with("num") && str[3] >= '1' && str[3] <= '9') {
			//WARNING HID_KEY_KEYPAD_0 is on the end of HID_KEY_KEYPAD_SEQUENCE
			//but ASCII 0 in on the top on numerical SEQUENCE
			int i = (str[3] - '1');
			debugIf(LOG_USB_DEVIVE, "num", i);
			combination.keyboardKey(uint8_t(HID_KEY_KEYPAD_1 + i));
			return true;
		}

		if (str == "num0"sv) {
			//WARNING HID_KEY_KEYPAD_0 is on the end of HID_KEY_KEYPAD_SEQUENCE
			//but ASCII 0 in on the top on numerical SEQUENCE
			combination.keyboardKey(HID_KEY_KEYPAD_0);
			return true;
		}

		if (str == "equal"sv || str == "plus") {
			combination.keyboardKey(HID_KEY_EQUAL);
			return true;
		}

		if (str == "minus"sv) {
			combination.keyboardKey(HID_KEY_MINUS);
			return true;
		}

		if (str == "comma"sv) {
			combination.keyboardKey(HID_KEY_COMMA);
			return true;
		}


		if (str == "period"sv) {
			combination.keyboardKey(HID_KEY_PERIOD);
			return true;
		}

		if (str == "slash"sv) {
			combination.keyboardKey(HID_KEY_SLASH);
			return true;
		}


		if (str == "f10"sv) {
			combination.keyboardKey(HID_KEY_F10);
			return true;
		}

		if (str == "f11"sv) {
			combination.keyboardKey(HID_KEY_F11);
			return true;
		}

		if (str == "f12"sv) {
			combination.keyboardKey(HID_KEY_F12);
			return true;
		}

		if (str.size() == 2 && str[0] == 'f' && str[1] >= '1' && str[1] <= '9') {
			int i = (str[1] - '0') - 1;
			combination.keyboardKey(uint8_t(HID_KEY_F1 + i));
			return true;
		}

		if (str == "tab"sv) {
			combination.keyboardKey(HID_KEY_TAB);
			return true;
		}

		if (str == "enter"sv) {
			combination.keyboardKey(HID_KEY_ENTER);
			return true;
		}

		if (str == "backspace"sv || str == "back"sv) {
			combination.keyboardKey(HID_KEY_BACKSPACE);
			return true;
		}

		if (str == "space"sv) {
			combination.keyboardKey(HID_KEY_SPACE);
			return true;
		}

		if (str == "tilde"sv) {
			combination.keyboardKey(HID_KEY_ESCAPE);
			return true;
		}

		if (str.size() == 1) {
			//probably encoded keyboardKey like +b:long+
			combination.keyboardSymbol(str.front());
			return true;
		}

		return false;
	}

	bool keyboard::executeOn(
		hid::composite::combination_writer_type& combination,
		const word_type& word,
		context_type& ctx
	)  {
		try {
			return specialKey(combination, word.dataView, word);
		} catch (std::exception& e) {
			error("keyboard exception", e.what());
			return false;
		}
	}

}
