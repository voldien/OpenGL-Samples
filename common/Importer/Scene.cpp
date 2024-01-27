#include "Scene.h"
#include "GLHelper.h"
#include "ModelImporter.h"
#include <GL/glew.h>

namespace glsample {

	Scene::~Scene() {}

	void Scene::release() {
		for (size_t i = 0; i < this->refGeometry.size(); i++) {
			if (glIsVertexArray(this->refGeometry[i].vao)) {
				glDeleteVertexArrays(1, &this->refGeometry[i].vao);
			}
			if (glIsBuffer(this->refGeometry[i].ibo)) {
				glDeleteBuffers(1, &this->refGeometry[i].ibo);
			}
			if (glIsBuffer(this->refGeometry[i].vbo)) {
				glDeleteBuffers(1, &this->refGeometry[i].vao);
			}
		}

		for (size_t i = 0; i < this->refTexture.size(); i++) {
			glDeleteTextures(1, &this->refTexture[i].texture);
		}
	}

	void Scene::update(const float deltaTime) {

		if (normalDefault == 0) {
			/*	Create white texture.	*/
			glGenTextures(1, &this->normalDefault);
			glBindTexture(GL_TEXTURE_2D, this->normalDefault);
			const unsigned char white[] = {127, 127, 255, 255};
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			/*	No Mipmap.	*/
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);
		}

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
				const TextureAssetObject *tex = &this->refTexture[material.normalIndex];

				if (tex && tex->texture > 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, tex->texture);
				} else {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, this->normalDefault);
				}
			} else {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, this->normalDefault);
			}

			/*	*/
			if (material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size()) {
				const TextureAssetObject &tex = this->refTexture[material.maskTextureIndex];
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, tex.texture);
			} else {
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glPolygonMode(GL_FRONT_AND_BACK, material.wireframe_mode ? GL_LINE : GL_FILL);

			/*	*/
			const bool useBlending = material.opacity < 1.0f || material.maskTextureIndex >= 0;

			if (useBlending) {
				glEnable(GL_BLEND);
				/*	*/
				glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
				glBlendEquation(GL_FUNC_ADD);
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

			glBindVertexArray(this->refGeometry[0].vao);

			/*	*/
			glDrawElementsBaseVertex(
				GL_TRIANGLES, this->refGeometry[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
				(void *)(sizeof(unsigned int) * this->refGeometry[node->geometryObjectIndex[i]].indices_offset),
				this->refGeometry[node->geometryObjectIndex[i]].vertex_offset);

			glBindVertexArray(0);
		}
	}

	void Scene::sortRenderQueue() {}

} // namespace glsample