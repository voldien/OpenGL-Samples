#pragma once
#include "GLSampleSession.h"
#include "ModelImporter.h"

namespace glsample {

	class FVDECLSPEC ImportHelper {
	  public:
		/**
		 * @brief
		 *
		 * @param modelLoader
		 * @param modelSet
		 */
		static void loadModelBuffer(ModelImporter &modelLoader, std::vector<GeometryObject> &modelSet);

		/**
		 * @brief
		 *
		 * @param modelLoader
		 * @param textures
		 */
		static void loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures);
	};
} // namespace glsample