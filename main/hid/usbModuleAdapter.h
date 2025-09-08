#pragma once

#include "usbDevice.h"
#include <functional>

namespace hid {
	
	//main meaning of this class is alternative to inheritance from usbModule
	class usbModuleAdapter final : public virtual usbModule {
		
		public:
		
			typedef std::function<bool(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)> setReportT;
			typedef std::function<uint16_t(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)> 	 getReportT;
			
		private:
		
			const setReportT setReportCb;
			const getReportT getReportCb;
			
		public:
			
			usbModuleAdapter(setReportT setCallback, getReportT getCallback): setReportCb(setCallback), getReportCb(getCallback) {}
			virtual 	~usbModuleAdapter() = default;
			
			bool 		setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 	override;
			uint16_t 	getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) 		override;
	};

}
	
