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

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC IOUtil {
	  public:
		static std::vector<char> readFileString(const std::string &filename, IFileSystem *filesystem) {

			Ref<IO> ref = Ref<IO>(filesystem->openFile(filename.c_str(), IO::IOMode::READ));
			std::vector<char> string = fragcore::IOUtil::readString<char>(ref);
			ref->close();
			return string;
		}

		static std::vector<char> readFileString(const std::string &filename) {
			return readFileString(filename, FileSystem::getFileSystem());
		}

		template <typename T> static std::vector<T> readFileData(const std::string &filename, IFileSystem *filesystem) {

			Ref<IO> ref = Ref<IO>(filesystem->openFile(filename.c_str(), IO::IOMode::READ));
			std::vector<T> buffer = fragcore::IOUtil::readFile<T>(ref);
			ref->close();
			return buffer;
		}
	};
} // namespace glsample

#endif
