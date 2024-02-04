#pragma once
#include "Core/UIDObject.h"
#include "GLSampleSession.h"
#include "ImportHelper.h"
#include "ModelImporter.h"

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Scene : public fragcore::UIDObject {
	  public:
		unsigned int normalDefault = 0;
		Scene() {}
		virtual ~Scene();

		virtual void release();

		virtual void update(const float deltaTime);

		// virtual void updateBuffers();

		virtual void render();

		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

	  public:
		const std::vector<NodeObject *> &getNodes() const noexcept { return this->nodes; }
		const std::vector<GeometryObject> &getMeshes() const noexcept { return this->refGeometry; }
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