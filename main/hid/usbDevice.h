#pragma once

#include <sys/_stdint.h>
#include <memory>

#include "generated.h"

#include "tinyusb.h"
#include "class/hid/hid_device.h"




// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance);

extern const tusb_desc_device_t descriptor_dev_kconfig;
extern const char *descriptor_str_kconfig[];

namespace hid {
	
	size_t sizeof_string_desciptor_structure(const char** begining);

	class usbModule {
		public:
			virtual ~usbModule() = default;
			virtual bool 			setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 	= 0;
			virtual uint16_t 		getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) 		= 0;
	};
	
	class usbDeviceModule: public virtual usbModule {
		public:
			virtual ~usbDeviceModule() = default;
			virtual uint8_t const 				*descriptorReport(uint8_t instance) = 0;
			virtual const tusb_desc_device_t 	*deviceDescriptor() 				= 0;
			virtual const char 					**stringDescriptor() 				= 0;
			virtual const uint8_t 				*configurationDescriptor() 			= 0;
	};
	
/*	class cmdReceiverInterface: public virtual device {
		virtual bool setReport();
	};
	
	class cmdSenderInterface: public virtual device {
		virtual bool getReport();
	};*/
	

	class usbDevice: public usbDeviceModule {
		
		struct node {
			usbModule*  device;
			node* 	 	next;
		};
		
		static node* rootNode;
				
		std::unique_ptr<tinyusb_config_t> config = nullptr;
		
		protected:
		
			virtual tinyusb_config_t makeConfig();
		
		public:
		
			usbDevice(){};
		
			virtual    ~usbDevice() = default;

			void 		attach(usbModule* device);
			
			bool 		detach(usbModule* device);
			
			bool 		setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) override;
			
			uint16_t 	getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) override;
			
			uint8_t const 				*descriptorReport(uint8_t instance) override;
			
			const tusb_desc_device_t 	*deviceDescriptor() 				override;
			
			const char 					**stringDescriptor() 				override;
			
			const uint8_t 				*configurationDescriptor() 			override;
			
			bool 		install();
			
			bool 		deinstall();
			
	};
	
	extern std::unique_ptr<usbDevice> UsbDevice;
	
}
