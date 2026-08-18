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

				if constexpr(DEVICE_KB_DEVICE_DESC_OVERRIDE) {
					static tusb_desc_device_t device_descriptor = descriptor_dev_kconfig;
					device_descriptor.idVendor = idVendor();
					device_descriptor.idProduct = idProduct();
					return &device_descriptor;
				} else {
					return &descriptor_dev_kconfig;
				}
			}
			
			const char** stringDescriptor() override {
				if constexpr (DEVICE_KB_DEVICE_STR_DESC_OVERRIDE) {
					size_t string_descriptor_count = sizeof_string_desciptor_structure(descriptor_str_kconfig);
					static auto string_descriptor = new const char*[string_descriptor_count + 1]; //dynamic size
					memcpy(string_descriptor, descriptor_str_kconfig, sizeof(char*) * (string_descriptor_count + 1));
					string_descriptor[2] = descManufacturer();
					string_descriptor[3] = descSerials();
					return string_descriptor;
				} else {
					return descriptor_str_kconfig;
				}
			}
			
			const uint8_t *configurationDescriptor() override {
				return hid_configuration_descriptor;
			}

			inline static constexpr uint16_t idVendor() {
#ifdef DEVICE_KB_VENDORID
				return DEVICE_KB_VENDORID;
#else
				return descriptor_dev_kconfig.idVendor;
#endif
			}

			inline static constexpr uint16_t idProduct() noexcept {
#ifdef DEVICE_KB_PRODUCTID
				return DEVICE_KB_PRODUCTID;
#else
				return descriptor_dev_kconfig.idProduct;
#endif
			}

			inline static constexpr const char* descManufacturer() noexcept {
#ifdef DEVICE_KB_MANUFACTURER
				return DEVICE_KB_MANUFACTURER;
#else
				return descriptor_str_kconfig[2];
#endif
			}

			inline static constexpr const char* descSerials() noexcept {
#ifdef DEVICE_KB_SERIALS
				return DEVICE_KB_SERIALS;
#else
				return descriptor_str_kconfig[3];
#endif
			}
	};

};
