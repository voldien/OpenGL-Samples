#ifndef _VK_SAMPLE_IMAGE_IMPORTER_H_
#define _VK_SAMPLE_IMAGE_IMPORTER_H_ 1
#include "../GLSample.h"
#include <stdio.h>
#include <string>
#include <vector>

/**
 * @brief
 *
 */
class TextureImporter {
  public:
	TextureImporter(FileSystem *filesystem);

	int loadImage2D(const std::string &path);
	int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
					const std::string &pz, const std::string &nz);
	int loadCubeMap(const std::vector<std::string> &paths);

  private:
	FileSystem *filesystem;
};

#endif
