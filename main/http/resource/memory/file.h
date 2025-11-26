#pragma once

#include <ostream>

#include "codes.h"
#include "contentType.h"
#include "esp_err.h"
#include "resource.h"
#include "map.h"
#include "response.h"

namespace http::resource::memory {
	
	enum class endings {
		BINARY,
		TEXT
	};
	
	class file: public resource {
		
		const int 	addressStart;
		const int 	addressEnd;
		const int 	ending;
		const char* name;
		const char* contentType;
		const char* checksum;
		
		protected:
		
			handlerRes handle(request& req, response& resp, server& serv) override;
		
		public:
				
			friend response& operator<<(response& stream, const file& result);

			//file(int addressStart, int addressEnd, endings end, const char * name);
			
			file(int addressStart, int addressEnd, endings end, const char * name, const char * contentType, const char* checksum = nullptr);
			
			file(int addressStart, int addressEnd, endings end, const char * name, enum contentType ct, const char* checksum = nullptr);

            //is override needed, for now its for unification with other virtual function?
            ~file() override = default;
			
			bool operator==(const file& other) const;
			
			bool operator!=(const file& other) const;
			
			bool operator<(const file& other) const;
				
			int type() const override;
			
			handlerRes operator()(request& req, response& resp, server& serv);
			
	};
	
	response& operator<<(response& stream, const std::string_view& str);
}


