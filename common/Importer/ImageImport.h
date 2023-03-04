#pragma once
#include "../GLSample.h"
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
		TextureImporter(IFileSystem *filesystem);

		int loadImage2D(const std::string &path);
		int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
						const std::string &pz, const std::string &nz);
		int loadCubeMap(const std::vector<std::string> &paths);

	  private:
		IFileSystem *filesystem;
	};

} // namespace glsample