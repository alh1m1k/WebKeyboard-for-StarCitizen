
#include "resultStream.h"


std::ostream& operator<<(std::ostream& stream, const resBool& result)	{
	if (result) {
		stream << "success" << std::endl;
	} else {
		stream << "fail[" << result.code() << "]" << std::endl;
	}
	return stream;
}