#include "Scene.h"
#include "GLHelper.h"
#include "ModelImporter.h"
#include "imgui.h"
#include <GL/glew.h>
#include <iostream>
#include <ostream>

namespace glsample {

	Scene::Scene() { this->init(); }

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

		/*	*/
		for (size_t i = 0; i < this->refTexture.size(); i++) {
			glDeleteTextures(1, &this->refTexture[i].texture);
		}
	}

	void Scene::init() {

		const unsigned char normalForward[] = {127, 127, 255, 255};
		const unsigned char white[] = {255, 255, 255, 255};
		const unsigned char black[] = {0, 0, 0, 255};

		std::vector<const unsigned char *> arrs = {white, normalForward, white};
		std::vector<int *> texRef = {&this->default_textures[TextureType::Diffuse],
									 &this->default_textures[TextureType::Normal],
									 &this->default_textures[TextureType::AlphaMask]};

		for (size_t i = 0; i < arrs.size(); i++) {

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

		glGenBuffers(1, &this->node_uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->node_uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, 1 << 16, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void Scene::update(const float deltaTime) {

		/*	Update animations.	*/
		for (size_t x = 0; x < this->animations.size(); x++) {
		}
	}

	void Scene::render() {

		// TODO: sort materials and geometry.
		this->sortRenderQueue();

		// TODO: merge by shared geometries.

		/*	Iterate through each node.	*/

		//	for (size_t x = 0; x < this->renderQueue.size(); x++) {
		for (const NodeObject *node : this->renderQueue) {
			/*	*/
			// const NodeObject *node = this->renderQueue[x];
			this->renderNode(node);
		}
	}

	void Scene::bindTexture(const MaterialObject &material, const TextureType texture_type) {

		/*	*/
		if (material.texture_index[texture_type] >= 0 && material.texture_index[texture_type] < refTexture.size()) {

			const TextureAssetObject *tex = &this->refTexture[material.texture_index[texture_type]];

			if (tex && tex->texture > 0) {
				glActiveTexture(GL_TEXTURE0 + texture_type);
				glBindTexture(GL_TEXTURE_2D, tex->texture);
			} else {
				glActiveTexture(GL_TEXTURE0 + texture_type);
				glBindTexture(GL_TEXTURE_2D, this->default_textures[texture_type]);
			}
		} else {
			glActiveTexture(GL_TEXTURE0 + texture_type);
			glBindTexture(GL_TEXTURE_2D, this->default_textures[texture_type]);
		}
		/*	*/
	}

	void Scene::renderNode(const NodeObject *node) {

		for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

			/*	*/
			const MaterialObject &material = this->materials[node->materialIndex[i]];

			this->bindTexture(material, TextureType::Diffuse);
			this->bindTexture(material, TextureType::Normal);
			this->bindTexture(material, TextureType::AlphaMask);

			/*	*/
			glPolygonMode(GL_FRONT_AND_BACK, material.wireframe_mode ? GL_LINE : GL_FILL);
			/*	*/
			const bool useBlending = material.opacity < 1.0f; // material.maskTextureIndex >= 0;
			glDisable(GL_STENCIL_TEST);

			glDepthFunc(GL_LESS);

			/*	*/
			if (useBlending) {
				/*	*/
				glEnable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				/*	*/
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
				material.blend_func_mode;
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
				glCullFace(GL_BACK);
			} else {
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			}

			if (!fragcore::validateExistingProgram()) {
				std::cout << fragcore::getProgramValidateString();
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

		/*	*/
		for (size_t x = 0; x < this->nodes.size(); x++) {

			/*	*/
			const NodeObject *node = this->nodes[x];
			if (node->materialIndex.empty()) {
				continue;
			}

			/*	*/
			const bool validIndex = node->materialIndex[0] < this->materials.size();

			if (validIndex) {

				const MaterialObject *material = &this->materials[node->materialIndex[0]];
				assert(material);

				int priority = computeMaterialPriority(*material);

				const bool useBlending = material->opacity < 1.0f;
				//					(material->maskTextureIndex >= 0 && material->maskTextureIndex < refTexture.size());
				////
				//|| 					material->opacity < 1.0f; // || material->transparent.length() < 2;
				// material->opacity < 1.0f ||

				if (useBlending) {
					this->renderQueue.push_back(node);
				} else {
					this->renderQueue.push_front(node);
				}

			} else {
				std::cerr << "Invalid Material " << node->name << std::endl;
				// Invalid
				// this->renderQueue.push_front(node);
			}
		}
	}

	int Scene::computeMaterialPriority(const MaterialObject &material) const noexcept {
		const bool use_clipping = material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size();
		const bool useBlending = material.opacity < 1.0f;

		return useBlending * 1000 + use_clipping * 100;
	}

	void Scene::renderUI() {

		/*	*/
		if (ImGui::TreeNode("Nodes")) {
		}
	}

} // namespace glsample