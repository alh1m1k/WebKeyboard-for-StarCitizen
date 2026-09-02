#include "composite.h"

#include "class/hid/hid_device.h"

#include "compositeWriter.h"
#include "deviceDescriptor.h"
#include "generated.h"
#include "usbDevice.h"
#include "util.h"

#include <cstring>
#include <sys/_stdint.h>


namespace hid {
	
	using namespace std::literals;
					
	composite::composite(): compositeWriter<task_type>(task, 0){
		debugIf(LOG_USB_DEVICE, "hid::keyboard::keyboard");
	}
	
	
	composite::~composite() {
		debugIf(LOG_USB_DEVICE, "hid::keyboard::~keyboard");
		if (_installed) {
			deinstall();
		}
	}
	
	bool composite::install() {	
		
		debugIf(LOG_USB_DEVICE, "keyboard::install usb initialization");
		
		UsbDevice->attach(this);
		task = std::make_unique<compositeTask>();
		
	    debugIf(LOG_USB_DEVICE,  "keyboard::install usb initialization done");
		
		return _installed = true;
	}
	
	void composite::deinstall() {
		task = nullptr;
		UsbDevice->detach(this);
		_installed = false;
	}
	
	bool composite::mounted() const noexcept {
		if constexpr (!DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC) {
			return _installed && tud_mounted();
		} else {
			return _installed;
		}
	}
	
	bool composite::setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
		if (report_type == hid_report_type_t::HID_REPORT_TYPE_OUTPUT && report_id == HID_ITF_PROTOCOL_KEYBOARD) {
			if (bufsize == 1) {
				if (leds != *buffer) {
					uint8_t oldLeds = leds;
					leds = *buffer;
					if (ledStatusChangeCallback) {
						ledStatusChangeCallback(leds, oldLeds);
					}
				}
				infoIf(LOG_USB_DEVICE, "keyboard::setReport", (uint)*buffer);
			} else {
				error("keyboard::setReport invalid buffer size for led request", bufsize);
			}
			return true;
		}
		return false;
	}

	uint16_t composite::getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
		return 0;
	}

}