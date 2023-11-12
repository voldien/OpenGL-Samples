#pragma once
#include "GLSampleSession.h"
#include "ModelImporter.h"

namespace glsample {

	class FVDECLSPEC ImportHelper {
	  public:
		static void loadModelBuffer(ModelImporter &modelLoader, std::vector<GeometryObject> &modelSet);

		static void loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures);
	};
} // namespace glsample