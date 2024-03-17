#pragma once
#include "Core/UIDObject.h"
#include "GLSampleSession.h"
#include "ImportHelper.h"
#include "ModelImporter.h"

namespace glsample {

	enum class TextureType {
		Diffuse = 0,	/*	*/
		Normal = 1,		/*	*/
		Mask = 2,		/*	*/
		Specular = 3,	/*	*/
		Emission = 4,	/*	*/
		Reflection = 5, /*	*/
		Displacement,
	};

	enum class TexturePBRType {
		Albedo,
		Normal,
		Metal,
		Roughness,
		AmbientOcclusion,
		Displacement,
	};

	/**
	 * @brief
	 *
	 */
	class Scene : public fragcore::UIDObject {
	  public:
		Scene() { this->init(); }
		virtual ~Scene();

		virtual void init();

		virtual void release();

		virtual void update(const float deltaTime);

		// virtual void updateBuffers();

		virtual void render(); // TODO, add camera.

		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

	  public:
		const std::vector<NodeObject *> &getNodes() const noexcept { return this->nodes; }
		const std::vector<MeshObject> &getMeshes() const noexcept { return this->refGeometry; }

	  public:
		/*	TODO add queue structure.	*/

		std::vector<NodeObject *> nodes;
		std::vector<MeshObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<animation_object_t> animations;

	  protected:
		int normalDefault = -1;
		int diffuseDefault = -1;
		int emissionDefault = -1;

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