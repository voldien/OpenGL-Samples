#ifndef _VKCOMMON_IOTUTIL_H_
#define _VKCOMMON_IOTUTIL_H_ 1
#include <Exception.hpp>
#include <fmt/format.h>
#include <fstream>
#include <vector>

class IOUtil {
  public:
	static std::vector<char> readFile(const std::string &filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw cxxexcept::RuntimeException("failed to open file {}!", filename);
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}
};

#endif
