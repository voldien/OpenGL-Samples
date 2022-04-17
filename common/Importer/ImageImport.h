#ifndef _VK_SAMPLE_IMAGE_IMPORTER_H_
#define _VK_SAMPLE_IMAGE_IMPORTER_H_ 1
#include <stdio.h>
#include <string>
#include <vector>

/**
 * @brief
 *
 */
class TextureImporter {
  public:
	static int loadImage2D(const std::string &path);
	static int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
						   const std::string &pz, const std::string &nz);
	static int loadCubeMap(const std::vector<std::string> &paths);
};

#endif
