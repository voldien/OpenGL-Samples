#include "Scene.h"
#include "GLSampleSession.h"
#include "ImportHelper.h"
#include "ModelImporter.h"
#include <GL/glew.h>

namespace glsample {

	Scene::~Scene() {}

	void Scene::release() {
		for (size_t i = 0; i < this->refObj.size(); i++) {
		}

		for (size_t i = 0; i < this->refTexture.size(); i++) {
			glDeleteTextures(1, &this->refTexture[i].texture);
		}
	}

	void Scene::update(const float deltaTime) {

		/*	Update animations.	*/
		for (size_t x = 0; x < this->animations.size(); x++) {
		}
	}

	void Scene::render() {

		// TODO sort materials and geomtry.
		this->sortRenderQueue();

		/*	Iterate through each node.	*/
		for (size_t x = 0; x < this->nodes.size(); x++) {

			/*	*/
			const NodeObject *node = this->nodes[x];
			this->renderNode(node);
		}
	}

	static void bindTexture(MaterialObject &material, TextureAssetObject &texutre) {
		//	/*	*/
		//	if (material.normalIndex >= 0 && material.normalIndex < refTexture.size()) {
		//		const TextureAssetObject &tex = this->refTexture[material.normalIndex];
		//		glActiveTexture(GL_TEXTURE1);
		//		glBindTexture(GL_TEXTURE_2D, tex.texture);
		//	}
	}

	void Scene::renderNode(const NodeObject *node) {

		for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

			/*	*/
			const MaterialObject &material = this->materials[node->materialIndex[i]];

			if (material.diffuseIndex >= 0 && material.diffuseIndex < refTexture.size()) {
				const TextureAssetObject &diffuseTexture = this->refTexture[material.diffuseIndex];
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, diffuseTexture.texture);
			} else {
			}

			/*	*/
			if (material.normalIndex >= 0 && material.normalIndex < refTexture.size()) {
				const TextureAssetObject &tex = this->refTexture[material.normalIndex];
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, tex.texture);
			}

			/*	*/
			if (material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size()) {
				const TextureAssetObject &tex = this->refTexture[material.maskTextureIndex];
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, tex.texture);
			}

			glPolygonMode(GL_FRONT_AND_BACK, material.wireframe_mode ? GL_LINE : GL_FILL);

			/*	*/
			const bool useBlending = material.opacity < 1.0f || material.maskTextureIndex >= 0;

			if (useBlending) {
				glEnable(GL_BLEND);
				/*	*/
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

			} else {
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

			/*	*/
			if (material.culling_mode) {
				glDisable(GL_CULL_FACE);
			}
			glDisable(GL_CULL_FACE);

			glBindVertexArray(this->refObj[0].vao);

			/*	*/
			glDrawElementsBaseVertex(
				GL_TRIANGLES, this->refObj[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
				(void *)(sizeof(unsigned int) * this->refObj[node->geometryObjectIndex[i]].indices_offset),
				this->refObj[node->geometryObjectIndex[i]].vertex_offset);

			glBindVertexArray(0);
		}
	}

	void Scene::sortRenderQueue() {}

	Scene Scene::loadFrom(ModelImporter &importer) {

		Scene scene;

		/*	*/
		scene.nodes = importer.getNodes();
		ImportHelper::loadModelBuffer(importer, scene.refObj);
		ImportHelper::loadTextures(importer, scene.refTexture);
		scene.materials = importer.getMaterials();

		return scene;
	}
} // namespace glsample