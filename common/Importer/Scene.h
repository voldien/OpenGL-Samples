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

		// virtual void updateBuffers();

		virtual void render();

		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

	  private:
		/*	TODO add queue structure.	*/

		std::vector<NodeObject *> nodes;
		std::vector<GeometryObject> refObj;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<animation_object_t> animations;

	  public:
		// Template
		static Scene loadFrom(ModelImporter &importer);
		static void RenderUI(Scene &scene);
	};
} // namespace glsample