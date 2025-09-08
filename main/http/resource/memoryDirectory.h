#pragma once

#include <set>
#include <ostream>

#include "resource.h"

#include "codes.h"
#include "esp_err.h"
#include "resource.h"
#include "map.h"
#include "response.h"

namespace http::resource {
	
	class memoryDirectory: public resource {
		
		class slot {
			
			friend response& operator<<(response& stream, const slot& result); //in order to get access to slot
			
			std::string str;
			int 		address;
			size_t 		size;
			
			public:
			
				slot(std::string str);
				
				slot(std::string str, int address, size_t size);
				
				bool operator==(const slot& other) const;
				
				bool operator!=(const slot& other) const;
				
				bool operator<(const slot& other) const;
				
		};
		
		friend response& operator<<(response& stream, const slot& result); //in order to get access to slot
			
		std::set<slot> slots = {};
		
		protected:
		
			handlerRes handle(request& req, response& resp) override;
		
		public:
		
			friend response& operator<<(response& stream, const slot& result);
		
			memoryDirectory();
			
			memoryDirectory(std::initializer_list<slot> slots);
					
			int type() const override;
			
			handlerRes operator()(request& req, response& resp);
			
	};
	
	response& operator<<(response& stream, const std::string_view& str);
}


