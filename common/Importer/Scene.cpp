#include "Scene.h"
#include "GLHelper.h"
#include "ModelImporter.h"
#include <GL/glew.h>

namespace glsample {

	Scene::~Scene() {}

	void Scene::release() {
		/*	*/
		for (size_t i = 0; i < this->refGeometry.size(); i++) {
			if (glIsVertexArray(this->refGeometry[i].vao)) {
				glDeleteVertexArrays(1, &this->refGeometry[i].vao);
			}
			if (glIsBuffer(this->refGeometry[i].ibo)) {
				glDeleteBuffers(1, &this->refGeometry[i].ibo);
			}
			if (glIsBuffer(this->refGeometry[i].vbo)) {
				glDeleteBuffers(1, &this->refGeometry[i].vbo);
			}
		}

		for (size_t i = 0; i < this->refTexture.size(); i++) {
			glDeleteTextures(1, &this->refTexture[i].texture);
		}
	}

	void Scene::init() {

		const unsigned char normalForward[] = {127, 127, 255, 255};
		const unsigned char white[] = {255, 255, 255, 255};
		const unsigned char black[] = {0, 0, 0, 255};

		std::vector<const unsigned char *> arrs = {normalForward, white, black};
		std::vector<int *> texRef = {&this->normalDefault, &this->diffuseDefault, &this->roughnessSpecularDefault};

		for (int i = 0; i < arrs.size(); i++) {

			FVALIDATE_GL_CALL(glGenTextures(1, (GLuint *)texRef[i]));
			FVALIDATE_GL_CALL(glBindTexture(GL_TEXTURE_2D, *texRef[i]));
			FVALIDATE_GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, arrs[i]));
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			/*	Border clamped to max value, it makes the outside area.	*/
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

			/*	No Mipmap.	*/
			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
			glBindTexture(GL_TEXTURE_2D, 0);
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

		//	for (size_t x = 0; x < this->renderQueue.size(); x++) {
		for (const NodeObject *node : renderQueue) {

			/*	*/
			// const NodeObject *node = this->renderQueue[x];
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
				glActiveTexture(GL_TEXTURE0 + TextureType::Diffuse);
				glBindTexture(GL_TEXTURE_2D, diffuseTexture.texture);

			} else {
				glActiveTexture(GL_TEXTURE0 + TextureType::Diffuse);
				glBindTexture(GL_TEXTURE_2D, this->diffuseDefault);
			}

			/*	*/
			if (material.normalIndex >= 0 && material.normalIndex < refTexture.size()) {
				const TextureAssetObject *tex = &this->refTexture[material.normalIndex];

				if (tex && tex->texture > 0) {
					glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
					glBindTexture(GL_TEXTURE_2D, tex->texture);
				} else {
					glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
					glBindTexture(GL_TEXTURE_2D, this->normalDefault);
				}
			} else {
				glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
				glBindTexture(GL_TEXTURE_2D, this->normalDefault);
			}

			/*	*/
			if (material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size()) {
				const TextureAssetObject &tex = this->refTexture[material.maskTextureIndex];
				glActiveTexture(GL_TEXTURE0 + TextureType::AlphaMask);
				glBindTexture(GL_TEXTURE_2D, tex.texture);
			} else {
				glActiveTexture(GL_TEXTURE0 + TextureType::AlphaMask);
				glBindTexture(GL_TEXTURE_2D, this->diffuseDefault);
			}

			/*	*/
			glPolygonMode(GL_FRONT_AND_BACK, material.wireframe_mode ? GL_LINE : GL_FILL);
			/*	*/
			const bool useBlending = material.opacity < 1.0f || material.maskTextureIndex >= 0;

			/*	*/
			if (useBlending) {
				glEnable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				/*	*/
				glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
				glBlendEquation(GL_FUNC_ADD);

			} else {
				/*	*/
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

			/*	*/
			if (material.culling_both_side_mode) {
				glDisable(GL_CULL_FACE);
			} else {
				glDisable(GL_CULL_FACE);
				glEnable(GL_CULL_FACE);
			}

			const MeshObject &refMesh = this->refGeometry[node->geometryObjectIndex[i]];
			glBindVertexArray(refMesh.vao);

			/*	*/
			glDrawElementsBaseVertex(refMesh.primitiveType, refMesh.nrIndicesElements, GL_UNSIGNED_INT,
									 (void *)(sizeof(unsigned int) * refMesh.indices_offset), refMesh.vertex_offset);

			glBindVertexArray(0);
		}
	}

	void Scene::sortRenderQueue() {

		this->renderQueue.clear();

		for (size_t x = 0; x < this->nodes.size(); x++) {

			/*	*/
			const NodeObject *node = this->nodes[x];
			const bool validIndex = node->materialIndex[0] < this->materials.size();

			if (validIndex) {
				const MaterialObject *material = &this->materials[node->materialIndex[0]];
				assert(material);

				const bool useBlending = material->opacity < 1.0f || material->maskTextureIndex >= 0;
				if (useBlending) {
					this->renderQueue.push_back(node);
				} else {
					this->renderQueue.push_front(node);
				}
			} else {
				this->renderQueue.push_front(node);
			}
		}
	}

} // namespace glsample