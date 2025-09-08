#include "usbModuleAdapter.h"

namespace hid {
	
	bool usbModuleAdapter::setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
		return setReportCb(instance, report_id, report_type, buffer, bufsize);
	}
	
	uint16_t usbModuleAdapter::getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
		return getReportCb(instance, report_id, report_type, buffer, reqlen);
	}

}