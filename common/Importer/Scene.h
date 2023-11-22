#pragma once
#include "GLSampleSession.h"
#include "ImportHelper.h"
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

	  public:
		/*	TODO add queue structure.	*/

		std::vector<NodeObject *> nodes;
		std::vector<GeometryObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<animation_object_t> animations;

	  public:
		// Template

		template <typename T = Scene> static T loadFrom(ModelImporter &importer) {

			T scene;

			/*	*/
			scene.nodes = importer.getNodes();
			ImportHelper::loadModelBuffer(importer, scene.refGeometry);
			ImportHelper::loadTextures(importer, scene.refTexture);
			scene.materials = importer.getMaterials();

			return scene;
		}
		static void RenderUI(Scene &scene);
	};
} // namespace glsample