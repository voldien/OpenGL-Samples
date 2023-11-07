#pragma once
#include "ModelImporter.h"

namespace glsample {
	class Scene {
	  public:
		Scene() {}
		virtual ~Scene();

		virtual void update(const float deltaTime);
		virtual void render();

		virtual void renderNode(const NodeObject *node);

	  private:
		std::vector<NodeObject *> nodes;
		std::vector<GeometryObject> refObj;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;

	  public:
		static Scene loadFrom(ModelImporter &importer);
		static void RenderUI(Scene &scene);
	};
} // namespace glsample