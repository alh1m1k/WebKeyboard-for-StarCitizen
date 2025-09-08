#pragma once

#include "generated.h"

#include "usbDevice.h"
#include "deviceDescriptor.h"
#include "util.h"


namespace hid {

	class UsbDeviceImpl: public hid::usbDevice {
		
		public: 
		
			const uint8_t *descriptorReport(uint8_t instance) override{
				return hid_report_descriptor;
			}
			
			const tusb_desc_device_t *deviceDescriptor() override {
								
	#ifdef  DEVICE_KB_DEVICE_DESC_OVERRIDED
				static tusb_desc_device_t device_descriptor = descriptor_dev_kconfig;
			
	#ifdef  DEVICE_KB_VENDORID
				device_descriptor.idVendor 	= DEVICE_KB_VENDORID;
	#endif
	
	#ifdef  DEVICE_KB_PRODUCTID
				device_descriptor.idProduct = DEVICE_KB_PRODUCTID;
	#endif
	
				return &device_descriptor;
	#endif
	
				return &descriptor_dev_kconfig;
	
			}
			
			const char** stringDescriptor() override {
								
	#ifdef  DEVICE_KB_DEVICE_STR_DESC_OVERRIDED
			
				size_t string_descriptor_count = sizeof_string_desciptor_structure(descriptor_str_kconfig);
				static auto string_descriptor = new const char*[string_descriptor_count + 1];
				memcpy(string_descriptor, descriptor_str_kconfig, sizeof(char*) * (string_descriptor_count + 1));
			
	#ifdef  DEVICE_KB_MANUFACTURER
				string_descriptor[1] 	= DEVICE_KB_MANUFACTURER;
	#endif
			
	#ifdef  DEVICE_KB_PRODUCT
				string_descriptor[2] 	= DEVICE_KB_PRODUCT;
	#endif
	
	#ifdef  DEVICE_KB_SERIALS
				string_descriptor[3] 	= DEVICE_KB_SERIALS;
	#endif
	
				return string_descriptor;
	#endif
				return descriptor_str_kconfig;
			}
			
			const uint8_t *configurationDescriptor() override {
				return hid_configuration_descriptor;
			}
	};

};
