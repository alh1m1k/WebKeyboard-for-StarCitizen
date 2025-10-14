#include "usbDevice.h"

#include <stdlib.h>
#include <sys/_stdint.h>
#include <limits>

#include "tinyusb.h"
#include "class/hid/hid_device.h"

#include "../util.h"
#include "deviceDescriptor.h"
#include "generated.h"





// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
	debugIf(LOG_KEYBOARD || LOG_JOYSTICK, "tud_hid_get_report_cb: ", instance);
    return hid::UsbDevice->getReport(instance, report_id, report_type, buffer, reqlen);
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
	debugIf(LOG_KEYBOARD || LOG_JOYSTICK, "tud_hid_set_report_cb instance: ", (uint)instance, " report: ", (uint)report_id, " type: ", (uint)report_type, " buffer: ", (uint)*buffer, " size: ",  (uint)bufsize);
	hid::UsbDevice->setReport(instance, report_id, report_type, buffer, bufsize);
}

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) 
{
	debugIf(LOG_KEYBOARD || LOG_JOYSTICK, "tud_hid_descriptor_report_cb: ", instance);
	return hid::UsbDevice->descriptorReport(instance);
}



namespace hid {
	
	usbDevice::node* usbDevice::rootNode = nullptr;
	
	size_t sizeof_string_desciptor_structure(const char** begining) {
		if (begining == nullptr) {
			return 0;
		}
		size_t string_descriptor_count = 0;
		while (descriptor_str_kconfig[++string_descriptor_count] != NULL);
		return string_descriptor_count;
	}
	
	void usbDevice::attach(usbModule* device) {
		if (usbDevice::rootNode == nullptr) {
			usbDevice::rootNode = new usbDevice::node{ .device = device, .next = nullptr };
		} else {
			for (node* nd = usbDevice::rootNode; ; nd = nd->next) {
				if (nd->next == nullptr) {
					nd->next = new usbDevice::node{ .device = device, .next = nullptr };
					break;
				}
			}
		}
	}
	
	bool usbDevice::detach(usbModule* device) {
		//When declaring pointers, the * belongs to the name, not the type.
		for (node* nd = usbDevice::rootNode, *prev = nullptr; nd != nullptr; prev = nd, nd = nd->next) {
			if (nd->device == device) {
				if (prev == nullptr) {
					usbDevice::rootNode = nd->next;
				} else {
					prev->next = nd->next;
				}
				delete nd;
				return true;
			}
		}
		return false;
	}
	
/**		
	error: 'dynamic_cast' not permitted with '-fno-rtti'

	for (node* nd = usbDevice::rootNode; nd != nullptr; nd = nd->next) {
		if (auto device = dynamic_cast<cmdReceiverInterface*>(nd->device); device != nullptr) {
			return true;
		}
	}
		
*/
	
	bool usbDevice::setReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
		for (node* nd = usbDevice::rootNode; nd != nullptr; nd = nd->next) {
			if (nd->device->setReport(instance, report_id, report_type, buffer, bufsize)) {
				return true;
			}
		}
		return false;
	}
	
	uint16_t usbDevice::getReport(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
		for (node* nd = usbDevice::rootNode; nd != nullptr; nd = nd->next) {
			if (auto size = nd->device->getReport(instance, report_id, report_type, buffer, reqlen); size) {
				return size;
			}
		}
		return 0;
	}
	
/*	uint16_t usbDevice::descriptorReport(uint8_t instance, uint8_t* buffer, uint16_t bufsize) {
		
		if (!!buffer != !!bufsize) {
			//actually buffer != nullptr and size == 0 is valid but make sense only as side effect 
			return 0;
		}
		
		uint32_t total = 0; 
		if (buffer == nullptr) {
			//buffer size calculation
			for (node* nd = usbDevice::rootNode; nd != nullptr; nd = nd->next) {
				auto chunk= nd->device->descriptorReport(instance, nullptr, 0);
				if (chunk == 0) {
					error("usbDevice::descriptorReport(size) node.descriptorReport return zero");
					return 0;
				}
				total += chunk;
			}
		} else {
			for (node* nd = usbDevice::rootNode; nd != nullptr; nd = nd->next) {
				if (auto chunk = nd->device->descriptorReport(instance, &buffer[total], bufsize - total); !chunk) {
					error("usbDevice::descriptorReport node.descriptorReport return zero");
					return 0;
				} else if (total += chunk; total > bufsize) {
					//if total > bufsize it is buffer overrun
					//if total == bufsize it is end or other node return 0
					error("usbDevice::descriptorReport buffer ovf", bufsize, total);
					return 0;
				}
			}
		}
		
		if (total > std::numeric_limits<uint16_t>::max()) {
			return 0;
		}
		
		return total;
	}*/
	
/*	#ifdef  DEVICE_KB_DEVICE_DESC_OVERRIDED
		device_descriptor = std::make_unique_for_overwrite<tusb_desc_device_t>();
		*device_descriptor = descriptor_dev_kconfig;
		
#ifdef  DEVICE_KB_VENDORID
		device_descriptor->idVendor 	= DEVICE_KB_VENDORID;
#endif

#ifdef  DEVICE_KB_PRODUCTID
		device_descriptor->idProduct 	= DEVICE_KB_PRODUCTID;
#endif

#endif

#ifdef  DEVICE_KB_DEVICE_STR_DESC_OVERRIDED
		size_t string_descriptor_count = sizeof_string_desciptor_structure(descriptor_str_kconfig);
		debug("keyboard::keyboard sizeof ", string_descriptor_count);
		
		string_descriptor = std::make_unique<const char*[]>( string_descriptor_count + 1 );
		memcpy(string_descriptor.get(), descriptor_str_kconfig, sizeof(char*) * (string_descriptor_count + 1));
		
		debug("keyboard::keyboard ", string_descriptor[1], " ", DEVICE_KB_MANUFACTURER);
#ifdef  DEVICE_KB_MANUFACTURER
		string_descriptor[1] 	= DEVICE_KB_MANUFACTURER;
#endif
		
		debug("keyboard::keyboard ", string_descriptor[2],  " ", DEVICE_KB_PRODUCT);
#ifdef  DEVICE_KB_PRODUCT
		string_descriptor[2] 	= DEVICE_KB_PRODUCT;
#endif

#ifdef  DEVICE_KB_SERIALS
		string_descriptor[3] 	= DEVICE_KB_SERIALS;
#endif

#endif*/


	uint8_t const *usbDevice::descriptorReport(uint8_t instance) {
		return nullptr;
	}
	
	const tusb_desc_device_t *usbDevice::deviceDescriptor() {
		return nullptr;
	}
			
	const char **usbDevice::stringDescriptor() {
		return nullptr;
	}
	
	const uint8_t *usbDevice::configurationDescriptor() {
		return nullptr;
	}
	
	tinyusb_config_t usbDevice::makeConfig() {
		auto stringDecr = stringDescriptor();
		debugIf(LOG_KEYBOARD || LOG_JOYSTICK, "usbDevice::makeConfig str cnt", (int)sizeof_string_desciptor_structure(stringDecr));
		return {
	        .device_descriptor = deviceDescriptor(),
	        .string_descriptor = stringDecr,
	        .string_descriptor_count = (int)sizeof_string_desciptor_structure(stringDecr),
	        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
	        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
	        .hs_configuration_descriptor = hid_configuration_descriptor,
	        .qualifier_descriptor = NULL,
#else
	        .configuration_descriptor = configurationDescriptor(),
#endif // TUD_OPT_HIGH_SPEED
			.self_powered 		= false,
			.vbus_monitor_io 	= 0,
	    };
	}
	
	
	bool usbDevice::install() {

        config = std::make_unique_for_overwrite<tinyusb_config_t>();
        *config = makeConfig();

#ifdef DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC
        info("DEBUG_ALLOW_JTAG_VIA_SUPPRESSED_CDC are enabled, usb stack supressed");
#else
        ESP_ERROR_CHECK(tinyusb_driver_install(config.get()));
#endif
	    
	    return true;
	}
	
	bool usbDevice::deinstall() {
		if (config == nullptr) {
			return false;
		}
		error("usbDevice::deinstall() not possible/ not impl");
		return false;
	}
	
	std::unique_ptr<usbDevice> UsbDevice = std::make_unique<usbDevice>();
	
}
