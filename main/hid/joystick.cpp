#include "joystick.h"

#include "util.h"
#include <sys/_stdint.h>

namespace hid {

	joystick::joystick(): state({AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIDDLE, AXIS_MIN, AXIS_MIN, 0}) {
		debugIf(LOG_JOYSTICK, "hid::joystick::joystick");
	}
	
	
	joystick::~joystick() {
		debugIf(LOG_JOYSTICK, "hid::joystick::~joystick");
		if (_installed) {
			deinstall();
		}
	}
	
	bool joystick::install() {	
		
		debugIf(LOG_JOYSTICK,  "joystick::install usb initialization");
		
		UsbDevice->attach(this);
		
	    debugIf(LOG_JOYSTICK,  "joystick::install usb initialization DONE");
		
		return true;
	}
	
	bool joystick::writeAxis(const int axisId, uint16_t value) {
		 if (axisId >= 0 && axisId <= (int)axis::THROTTLE) {
			((uint16_t*)&state)[axisId] = value;
			writeAxis();
			return true;
		 }
		 return false;
	}
	
	void joystick::writeAxis() {
		infoIf(LOG_JOYSTICK, "axisx", 	(uint)state.x);
		infoIf(LOG_JOYSTICK, "axisy", 	(uint)state.y);
		infoIf(LOG_JOYSTICK, "axisrx", 	(uint)state.rx);
		infoIf(LOG_JOYSTICK, "axisry", 	(uint)state.ry);
		infoIf(LOG_JOYSTICK, "axisrx", 	(uint)state.rx);
		infoIf(LOG_JOYSTICK, "axisry", 	(uint)state.ry);
		infoIf(LOG_JOYSTICK, "axisls", 	(uint)state.ls);
		infoIf(LOG_JOYSTICK, "axisrs", 	(uint)state.rs);
		infoIf(LOG_JOYSTICK, "axisld", 	(uint)state.ld);
		infoIf(LOG_JOYSTICK, "axisrd", 	(uint)state.rd);
		infoIf(LOG_JOYSTICK, "buttons", (uint32_t)state.buttons);
#ifdef DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC
        info("DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC are enabled, usb stack supressed");
#else
        tud_hid_report(REPORT_ID_GAMEPAD, &state, sizeof(state));
#endif
		commandCounter++;
	}
		
	void joystick::deinstall() {
		UsbDevice->detach(this);
	}
	
	bool joystick::mounted() const noexcept {
#ifdef DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC
        return _installed;
#else
        return _installed && tud_mounted();
#endif
	}
	
	bool joystick::setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
		return false;
	}

	uint16_t joystick::getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
		return 0;
	}		
}