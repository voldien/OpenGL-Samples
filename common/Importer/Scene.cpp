#include "Scene.h"
#include "GLSampleSession.h"
#include "ImportHelper.h"
#include "ModelImporter.h"
#include <GL/glew.h>

namespace glsample {

	Scene::~Scene() {}

	void Scene::update(const float deltaTime) {}

	void Scene::render() {
		for (size_t x = 0; x < this->nodes.size(); x++) {

			/*	*/
			const NodeObject *node = this->nodes[x];
			this->renderNode(node);
		}
	}

	void Scene::renderNode(const NodeObject *node) {

		for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

			/*	*/
			const MaterialObject &material = this->materials[node->materialIndex[i]];

			if (material.diffuseIndex >= 0 && material.diffuseIndex < refTexture.size()) {
				TextureAssetObject &diffuseTexture = this->refTexture[material.diffuseIndex];
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, diffuseTexture.texture);
			} else {
			}

			if (material.normalIndex >= 0 && material.normalIndex < refTexture.size()) {
				TextureAssetObject &tex = this->refTexture[material.normalIndex];
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, tex.texture);
			}

			glBindVertexArray(this->refObj[0].vao);

			/*	*/
			glDrawElementsBaseVertex(
				GL_TRIANGLES, this->refObj[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
				(void *)(sizeof(unsigned int) * this->refObj[node->geometryObjectIndex[i]].indices_offset),
				this->refObj[node->geometryObjectIndex[i]].vertex_offset);

			glBindVertexArray(0);
		}
	}

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