#include "Scene.h"
#include "../Common.h"
#include "Math3D/Color.h"
#include "ModelImporter.h"
#include "UIComponent.h"
#include "Util/CameraController.h"
#include "imgui.h"
#include <GL/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <ostream>
#include <sys/types.h>

namespace glsample {

	class SceneSettingComponent : public nekomimi::UIComponent {
	  public:
		SceneSettingComponent(Scene &base) : scene(base) {}
		void draw() override {}

	  private:
		Scene &scene;
	};

	Scene::Scene() {
		this->init();
		glCullFace(GL_BACK);
	}

	Scene::~Scene() = default;

	void Scene::release() {

		/*	*/
		for (size_t geo_index = 0; geo_index < this->refGeometry.size(); geo_index++) {
			if (glIsVertexArray(this->refGeometry[geo_index].vao)) {
				glDeleteVertexArrays(1, &this->refGeometry[geo_index].vao);
			}
			if (glIsBuffer(this->refGeometry[geo_index].ibo)) {
				glDeleteBuffers(1, &this->refGeometry[geo_index].ibo);
			}
			if (glIsBuffer(this->refGeometry[geo_index].vbo)) {
				glDeleteBuffers(1, &this->refGeometry[geo_index].vbo);
			}
		}

		/*	*/
		for (size_t tex_index = 0; tex_index < this->refTexture.size(); tex_index++) {
			if (glIsTexture(this->refTexture[tex_index].texture)) {
				glDeleteTextures(1, &this->refTexture[tex_index].texture);
			}
		}
	}

	void Scene::init() {

		const unsigned char normalForward[] = {127, 127, 255, 255};
		const unsigned char white[] = {255, 255, 255, 255};

		std::vector<int *> texRef = {&this->default_textures[TextureType::Diffuse],
									 &this->default_textures[TextureType::Normal],
									 &this->default_textures[TextureType::AlphaMask]};

		this->default_textures[TextureType::Diffuse] = glsample::Common::createColorTexture(
			1, 1, fragcore::Color(white[0] / 255.0f, white[1] / 255.0f, white[2] / 255.0f, white[3] / 255.0f));
		this->default_textures[TextureType::AlphaMask] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::Emission] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::Irradiance] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::AmbientOcclusion] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::DepthBuffer] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::Specular] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::Displacement] = this->default_textures[TextureType::Diffuse];
		this->default_textures[TextureType::Normal] =
			glsample::Common::createColorTexture(1, 1,
												 fragcore::Color(normalForward[0] / 255.0f, normalForward[1] / 255.0f,
																 normalForward[2] / 255.0f, normalForward[3] / 255.0f));

		/*	Default.	*/
		glCreateSamplers(samplers.size(), samplers.data());
		for (size_t sampler_index = 0; sampler_index < samplers.size(); sampler_index++) {
			glSamplerParameteri(this->samplers[sampler_index], GL_TEXTURE_WRAP_S, GL_REPEAT);
			glSamplerParameteri(this->samplers[sampler_index], GL_TEXTURE_WRAP_T, GL_REPEAT);
			glSamplerParameteri(this->samplers[sampler_index], GL_TEXTURE_WRAP_R, GL_REPEAT);
			glSamplerParameteri(this->samplers[sampler_index], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glSamplerParameteri(this->samplers[sampler_index], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameterf(this->samplers[sampler_index], GL_TEXTURE_LOD_BIAS, 0.0f);
			glSamplerParameterf(this->samplers[sampler_index], GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
		}

		/*	*/
		{
			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize = 0;
			GLint maxUniformBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);

			this->UBOStructure.common_size_align = Math::align<size_t>(sizeof(CommonConstantData), minMapBufferSize);
			this->UBOStructure.common_size_total_align =
				this->UBOStructure.common_size_align * UniformDataStructure::nrUniformBuffer;

			this->UBOStructure.common_offset = 0;

			this->UBOStructure.node_size_align = Math::align<size_t>(1 << 16, minMapBufferSize);
			this->UBOStructure.node_size_total_align =
				this->UBOStructure.node_size_align * UniformDataStructure::nrUniformBuffer;

			this->UBOStructure.material_align_size = Math::align<size_t>(4096 * sizeof(MaterialData), minMapBufferSize);
			this->UBOStructure.material_align_total_size =
				this->UBOStructure.material_align_size * UniformDataStructure::nrUniformBuffer;

			this->UBOStructure.light_align_size = Math::align<size_t>(sizeof(LightData), minMapBufferSize);
			this->UBOStructure.light_align_total_size =
				this->UBOStructure.light_align_size * UniformDataStructure::nrUniformBuffer;

			const size_t total_ubo_size = this->UBOStructure.node_size_total_align +
										  this->UBOStructure.common_size_total_align +
										  this->UBOStructure.material_align_total_size;

			this->UBOStructure.node_offset = this->UBOStructure.common_size_total_align;
			this->UBOStructure.material_offset =
				this->UBOStructure.common_size_total_align + this->UBOStructure.node_size_total_align;
			this->UBOStructure.light_offset = this->UBOStructure.common_size_total_align +
											  this->UBOStructure.node_size_total_align +
											  this->UBOStructure.material_align_total_size;

			/*	*/
			glGenBuffers(1, &this->UBOStructure.node_and_common_uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.node_and_common_uniform_buffer);
			if (glBufferStorage) {
				glBufferStorage(GL_UNIFORM_BUFFER, total_ubo_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
				unsigned char *pdata = (unsigned char *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, total_ubo_size,
																		 GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
																			 GL_MAP_FLUSH_EXPLICIT_BIT);

				this->stageCommonBuffer = (CommonConstantData *)&pdata[0];
				*this->stageCommonBuffer = CommonConstantData();

				this->stageNodeData = (NodeData *)&pdata[this->UBOStructure.common_size_total_align];
				this->stageMaterialData = (MaterialData *)&pdata[this->UBOStructure.common_size_total_align +
																 this->UBOStructure.node_size_total_align];

			} else {
				glBufferData(GL_UNIFORM_BUFFER, total_ubo_size, nullptr, GL_DYNAMIC_DRAW);
			}

			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	}

	void Scene::update(const float deltaTime) {

		/*	Update animations.	*/
		for (size_t anim_index = 0; anim_index < this->animations.size(); anim_index++) {
			/*	*/
			AnimationObject &animationClip = this->animations[anim_index];
			// this->animations[x].curves;
		}

		this->updateBuffers();
	}

	void Scene::updateBuffers() {

		/*	*/
		size_t node_index = 0;

		for (const NodeObject *node : this->renderQueue) {
			this->stageNodeData[node_index++].model = node->modelGlobalTransform;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.node_and_common_uniform_buffer);

		/*	Update constant data.	*/
		glFlushMappedBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.common_offset,
								 this->UBOStructure.common_size_align);

		/*	Update Node Data.	*/
		glFlushMappedBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_offset, node_index * sizeof(NodeData));

		/*	Update Materials.	*/
		size_t material_index = 0;
		for (; material_index < this->materials.size(); material_index++) {

			this->stageMaterialData[material_index].ambientColor = this->materials[material_index].ambient;
			this->stageMaterialData[material_index].diffuseColor = this->materials[material_index].diffuse;
			this->stageMaterialData[material_index].specular_roughness = glm::vec4(
				glm::vec3(this->materials[material_index].specular), this->materials[material_index].shinininess);
			this->stageMaterialData[material_index].emission = this->materials[material_index].emission;
			this->stageMaterialData[material_index].transparency = this->materials[material_index].transparent;
		}
		/*	Update Material.	*/
		glFlushMappedBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.material_offset,
								 material_index * sizeof(MaterialData));

		/*	Update Lights.	*/
		// glFlushMappedBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.light_offset,
		//						 this->UBOStructure.light_align_size);
	}

	void Scene::render(Camera *camera) {
		/* Optioncally populate */
		if (camera) {
			this->stageCommonBuffer->camera = *camera;
			CameraController *cameraController = (CameraController *)camera;
			this->stageCommonBuffer->camera = *cameraController;
			/*	*/
			this->stageCommonBuffer->proj[0] = camera->getProjectionMatrix();
		}

		std::vector<NodeObject *> mainCameraNodeQueue;
		/*	Frustum Culling.	*/
		if (true) {
			for (size_t x = 0; x < this->getNodes().size(); x++) {

				NodeObject *node = this->getNodes()[x];

				for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

					/*	Compute world space AABB.	*/
					const AABB aabb = GeometryUtility::computeBoundingBox(
						fragcore::AABB::createMinMax(
							Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
							Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2])),
						GLM2E<float, 4, 4>(node->modelGlobalTransform));

					if (true) {
						BoundingSphere sphere = BoundingSphere(aabb.getCenter(), aabb.getSize().norm());

						if (camera->intersectionSphere(sphere) == Frustum::In) {
							/*	*/
							mainCameraNodeQueue.push_back(node);
						}

					} else {

						if (camera->intersectionAABB(aabb) == Frustum::In) {
							/*	*/
							mainCameraNodeQueue.push_back(node);
						}
					}
				}
			}
		} else {
			mainCameraNodeQueue = this->getNodes();
		}

		/*	*/
		this->render();
	}

	void Scene::render() {

		// TODO: sort materials and geometry.
		this->sortRenderQueue();

		// TODO: merge by shared geometries.

		/*	Iterate through each node.	*/

		/*	*/
		glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.common_buffer_binding,
						  this->UBOStructure.node_and_common_uniform_buffer, this->UBOStructure.common_offset,
						  this->UBOStructure.common_size_align);

		for (auto it = this->renderQueue.begin(); it != this->renderQueue.end(); it++) {
			const NodeObject *node = (*it);
			this->renderNode(node);
		}

		if (this->debugMode & DebugMode::Wireframe) {
			/*	*/
			for (const NodeObject *node : this->renderQueue) {
				/*	*/
				// const NodeObject *node = this->renderQueue[x];
				// this->renderNode(node);
			}
		}

		/*	Reset some OpenGL States.	*/
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		glCullFace(GL_BACK);
	}

	void Scene::bindTexture(const MaterialObject &material, const TextureType texture_type) {

		/*	*/
		unsigned int textureMapIndex = texture_type;
		unsigned int texture_id = 0;

		unsigned int materialTextureIndex = material.texture_index[texture_type];
		if (materialTextureIndex >= 0 && materialTextureIndex < refTexture.size()) {

			const TextureAssetObject *tex = &this->refTexture[materialTextureIndex];

			if (tex && tex->texture > 0) {
				texture_id = tex->texture;
			} else {
				texture_id = this->default_textures[texture_type];
			}
			const MaterialTextureSampling &sampling = material.texture_sampling[materialTextureIndex];

		} else {
			texture_id = this->default_textures[texture_type];
		}
		/*	*/
		if (glBindTextures) {
			glBindTextures(textureMapIndex, 1, &texture_id);
		} else {
			glActiveTexture(GL_TEXTURE0 + textureMapIndex);
			glBindTexture(GL_TEXTURE_2D, texture_id);
		}

		glBindSampler(textureMapIndex, this->samplers[textureMapIndex]);
	}

	void Scene::renderNode(const NodeObject *node) {

		/*	*/
		glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_buffer_binding,
						  this->UBOStructure.node_and_common_uniform_buffer, this->UBOStructure.node_offset,
						  this->UBOStructure.node_size_align);

		for (size_t geo_index = 0; geo_index < node->geometryObjectIndex.size(); geo_index++) {

			/*	Setup material.	*/
			{
				const MaterialObject &material = this->materials[node->materialIndex[geo_index]];

				glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.material_buffer_binding,
								  this->UBOStructure.node_and_common_uniform_buffer,
								  this->UBOStructure.material_offset +
									  node->materialIndex[geo_index] * sizeof(MaterialData),
								  this->UBOStructure.material_align_size);

				this->bindTexture(material, TextureType::Diffuse);
				this->bindTexture(material, TextureType::Normal);
				this->bindTexture(material, TextureType::AlphaMask);
				this->bindTexture(material, TextureType::Emission);
				this->bindTexture(material, TextureType::AmbientOcclusion);
				this->bindTexture(material, TextureType::Displacement);
				this->bindTexture(material, TextureType::Specular);
				//	this->bindTexture(material, TextureType::Irradiance);
				this->bindTexture(material, TextureType::DepthBuffer);

				/*	*/
				const RenderQueue domain = getQueueDomain(material);

				glDisable(GL_STENCIL_TEST);
				glDepthFunc(GL_LESS);

				/*	*/
				glPolygonMode(GL_FRONT_AND_BACK, material.wireframe_mode ? GL_LINE : GL_FILL);

				/*	*/
				if (domain == RenderQueue::Transparent) {
					/*	*/
					glEnable(GL_BLEND);
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_FALSE);

					/*	*/
					material.blend_func_mode;
					// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				} else {
					/*	*/
					glDisable(GL_BLEND);
					glEnable(GL_DEPTH_TEST);
					glDepthMask(GL_TRUE);
				}

				/*	*/
				if (material.culling_both_side_mode) {
					glDisable(GL_CULL_FACE);
					// glCullFace(GL_BACK);
				} else {
					glEnable(GL_CULL_FACE);
					// glCullFace(GL_BACK);
				}
			}

			const MeshObject &refMesh = this->refGeometry[node->geometryObjectIndex[geo_index]];
			glBindVertexArray(refMesh.vao);

			/*	*/
			glDrawElementsBaseVertex(refMesh.primitiveType, refMesh.nrIndicesElements, GL_UNSIGNED_INT,
									 (void *)(sizeof(unsigned int) * refMesh.indices_offset), refMesh.vertex_offset);

			glBindVertexArray(0);
		}
	}

	void Scene::sortRenderQueue() {

		this->renderQueue.clear();
		this->renderBucketQueue.clear();

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

				const RenderQueue domain = getQueueDomain(*material);

				renderBucketQueue[domain].push_back(node);

				if (domain >= RenderQueue::Transparent) {
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

		/*	Sort Transparent Objects.	*/
		renderBucketQueue[RenderQueue::Transparent];

		/*	Sort based on Shared mesh objects.	*/
	}

	int Scene::computeMaterialPriority(const MaterialObject &material) const noexcept {
		const bool use_clipping = material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size();
		const bool useBlending = material.opacity < 1.0f;

		return useBlending * 1000 + use_clipping * 100;
	}

	RenderQueue Scene::getQueueDomain(const MaterialObject &material) const noexcept {
		const bool useBlending = material.transparent[3] < 1.0f;
		const bool useGeometryAlpha = material.clipping < 1;
		const bool useWireframe = material.wireframe_mode;

		if (useWireframe) {
			return RenderQueue::Overlay;
		}

		if (useBlending) {
			return RenderQueue::Transparent;
		}
		if (useGeometryAlpha) {
			return RenderQueue::AlphaTest;
		}

		return RenderQueue::Geometry;
	}

	void Scene::renderUI() {

		if (ImGui::CollapsingHeader("Scene Settings")) {

			/*	*/
			if (ImGui::CollapsingHeader("Global Rendering Settings")) {
				ImGui::ColorEdit4("Global Ambient Color", &this->stageCommonBuffer->renderSettings.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::TextUnformatted("Global Fog");
				ImGui::DragInt("Fog Type", (int *)&this->stageCommonBuffer->renderSettings.fogSettings.fogType);
				ImGui::ColorEdit4("Fog Color", &this->stageCommonBuffer->renderSettings.fogSettings.fogColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat("Fog Density", &this->stageCommonBuffer->renderSettings.fogSettings.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->stageCommonBuffer->renderSettings.fogSettings.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->stageCommonBuffer->renderSettings.fogSettings.fogStart);
				ImGui::DragFloat("Fog End", &this->stageCommonBuffer->renderSettings.fogSettings.fogEnd);
			}

			/*	*/
			// TODO: add tree structure
			if (ImGui::TreeNode("Nodes")) {

				for (size_t node_index = 0; node_index < nodes.size(); node_index++) {
					ImGui::PushID(node_index);

					ImGui::TextUnformatted(nodes[node_index]->name.c_str());
					if (ImGui::DragFloat3("Position", &nodes[node_index]->localPosition[0])) {
						glm::mat4 globaMat = nodes[node_index]->parent == nullptr
												 ? glm::mat4(1)
												 : nodes[node_index]->parent->modelGlobalTransform;
						nodes[node_index]->modelGlobalTransform =
							glm::translate(globaMat, nodes[node_index]->localPosition);
					}

					if (ImGui::DragFloat4("Rotation (Quat)", &nodes[node_index]->localRotation[0])) {
						nodes[node_index]->localRotation = glm::normalize(nodes[node_index]->localRotation);
						glm::mat4 globaMat = nodes[node_index]->parent == nullptr
												 ? glm::mat4(1)
												 : nodes[node_index]->parent->modelGlobalTransform;
						nodes[node_index]->modelGlobalTransform =
							glm::translate(globaMat, nodes[node_index]->localPosition) *
							glm::mat4_cast(nodes[node_index]->localRotation);
					}

					ImGui::PopID();
				}

				/*	*/
				ImGui::TreePop();
			}
			if (ImGui::BeginChild("Materials")) {
				size_t material_index = 0;
				for (; material_index < this->materials.size(); material_index++) {
					ImGui::PushID(material_index);
					ImGui::TextUnformatted(this->materials[material_index].name.c_str());

					ImGui::ColorEdit4("Ambient Color", &this->materials[material_index].ambient[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::ColorEdit4("Diffuse Color", &this->materials[material_index].diffuse[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::ColorEdit4("Transparent Color", &this->materials[material_index].transparent[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::ColorEdit4("Emission Color", &this->materials[material_index].emission[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::ColorEdit4("Reflective Color", &this->materials[material_index].reflectivity[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::ColorEdit4("Specular Color", &this->materials[material_index].specular[0],
									  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			if (ImGui::BeginChild("Textures")) {
			}
			ImGui::EndChild();
		}
	}

} // namespace glsample