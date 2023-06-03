#pragma once
#include <Core/IO/FileSystem.h>
#include <Core/IO/IFileSystem.h>
#include <ImageLoader.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace glsample {
	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC TextureImporter {
	  public:
		TextureImporter(fragcore::IFileSystem *filesystem);

		int loadImage2D(const std::string &path);
		int loadImage2DRaw(const fragcore::Image &image);
		int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
						const std::string &pz, const std::string &nz);
		int loadCubeMap(const std::vector<std::string> &paths);

	  private:
		fragcore::IFileSystem *filesystem;
	};

} // namespace glsample