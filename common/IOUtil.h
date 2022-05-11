#ifndef _GL_SAMPLE_IOTUTIL_H_
#define _GL_SAMPLE_IOTUTIL_H_ 1
#include <Core/IO/FileSystem.h>
#include <Core/IO/IOUtil.h>
#include <Exception.hpp>
#include <fmt/format.h>
#include <fstream>
#include <vector>

using namespace fragcore;

namespace glsample {

	class FVDECLSPEC IOUtil {
	  public:
		static std::vector<char> readFile(const std::string &filename) {

			Ref<IO> ref = Ref<IO>(FileSystem::getFileSystem()->openFile(filename.c_str(), IO::IOMode::READ));
			return fragcore::IOUtil::readString<char>(ref);
		}

		static std::vector<char> readFileData(const std::string &filename) {

			Ref<IO> ref = Ref<IO>(FileSystem::getFileSystem()->openFile(filename.c_str(), IO::IOMode::READ));
			return fragcore::IOUtil::readFile<char>(ref);
		}
	};
} // namespace glsample

#endif
