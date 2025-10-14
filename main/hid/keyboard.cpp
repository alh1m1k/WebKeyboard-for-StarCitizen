#include "keyboard.h"

#include "class/hid/hid_device.h"
	
#include "generated.h"
#include "usbDevice.h"
#include "deviceDescriptor.h"
#include "util.h"
#include "writer.h"

#include <cstring>
#include <sys/_stdint.h>


//todo remove me
/*#include "usbDevice.cpp"*/


namespace hid {
	
	using namespace std::literals;
					
	keyboard::keyboard(): writer<task_type>(task, 0){
		debugIf(LOG_KEYBOARD, "hid::keyboard::keyboard");
	}
	
	
	keyboard::~keyboard() {
		debugIf(LOG_KEYBOARD, "hid::keyboard::~keyboard");
		if (_installed) {
			deinstall();
		}
	}
	
	bool keyboard::install() {	
		
		debugIf(LOG_KEYBOARD, "keyboard::install usb initialization");
		
		UsbDevice->attach(this);
		task = std::make_unique<keyboardTask>();
		
	    debugIf(LOG_KEYBOARD,  "keyboard::install usb initialization done");
		
		return true;
	}
	
	void keyboard::deinstall() {
		task = nullptr;
		UsbDevice->detach(this);
	}
	
	bool keyboard::mounted() const noexcept {
#ifdef DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC
        return _installed;
#else
        return _installed && tud_mounted();
#endif
	}
	
	bool keyboard::setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
		if (report_type == hid_report_type_t::HID_REPORT_TYPE_OUTPUT && report_id == 1) {
			if (bufsize == 1) {
				if (leds != *buffer) {
					uint8_t oldLeds = leds;
					leds = *buffer;
					if (ledStatusChangeCallback) {
						ledStatusChangeCallback(leds, oldLeds);
					}
				}
				infoIf(LOG_KEYBOARD, "keyboard::setReport", (uint)*buffer);
			} else {
				error("keyboard::setReport invalid buffer size for led request", bufsize);
			}
			return true;
		}
		return false;
	}

	uint16_t keyboard::getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
		return 0;
	}	
}