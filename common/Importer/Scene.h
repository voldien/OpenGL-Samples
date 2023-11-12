#pragma once
#include "GLSampleSession.h"
#include "ModelImporter.h"

namespace glsample {
	
	/**
	 * @brief
	 *
	 */
	class Scene {
	  public:
		Scene() {}
		virtual ~Scene();

		virtual void release();

		virtual void update(const float deltaTime);
		virtual void render();

		virtual void renderNode(const NodeObject *node);

	  private:
		std::vector<NodeObject *> nodes;
		std::vector<GeometryObject> refObj;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;

	  public:
		// Template
		static Scene loadFrom(ModelImporter &importer);
		static void RenderUI(Scene &scene);
	};
} // namespace glsample