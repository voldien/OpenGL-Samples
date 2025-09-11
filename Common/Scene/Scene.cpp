#include "Scene.h"
#include "../Common.h"
#include "IO/IFileSystem.h"
#include "Importer/ModelImporter.h"
#include "Math3D/Color.h"
#include "RenderDesc.h"
#include "SampleHelper.h"
#include "Scene/CameraController.h"
#include "Scene/Frustum.h"
#include "Util/DebugDrawer.h"
#include <GL/glew.h>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <internal_object_type.h>
#include <iostream>
#include <omp.h>
#include <ostream>
#include <sys/types.h>

namespace glsample {

	Scene::Scene() : settingUI(*this) {}

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
		for (size_t tex_index = 0; tex_index < this->default_textures.size(); tex_index++) {
			if (glIsTexture(this->default_textures[tex_index])) {
				glDeleteTextures(1, &this->default_textures[tex_index]);
			}
		}
	}

	void Scene::init(IFileSystem *filesystem) {

		const bool hasInit = this->default_textures[TextureTypeBinding::Diffuse] > 0;

		if (hasInit) {
			return;
		}

		if (filesystem) {
			this->debugDrawer = new DebugDrawManager(filesystem);
		}
		/*	Create Internal shaders.	*/

		/*	*/
		// this->DirLightPool.resize(64);
		// this->PointLightPool.resize(64);
		this->visableNodes.reserve(2048);

		/*	Create default textures.	*/
		{
			const unsigned char white[] = {255, 255, 255, 255};
			const unsigned char black[] = {0, 0, 0, 255};

			this->default_textures[TextureTypeBinding::Diffuse] = glsample::CommonUtil::createColorTexture(
				1, 1, fragcore::Color(white[0] / 255.0f, white[1] / 255.0f, white[2] / 255.0f, white[3] / 255.0f));
			this->default_textures[TextureTypeBinding::AlphaMask] = this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::Emission] = this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::Irradiance] =
				this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::AmbientOcclusion] =
				this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::DepthBuffer] =
				this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::Specular_Roughness] =
				this->default_textures[TextureTypeBinding::Diffuse];
			this->default_textures[TextureTypeBinding::AmbientOcclusion] =
				this->default_textures[TextureTypeBinding::Diffuse];

			this->default_textures[TextureTypeBinding::Displacement] = glsample::CommonUtil::createColorTexture(
				1, 1, fragcore::Color(black[0] / 255.0f, black[1] / 255.0f, black[2] / 255.0f, black[3] / 255.0f));
			this->default_textures[TextureTypeBinding::Metal] = this->default_textures[TextureTypeBinding::Diffuse];

			/*	Default Normal.	*/
			this->default_textures[TextureTypeBinding::Normal] =
				glsample::CommonUtil::createColorTexture16F(1, 1, fragcore::Color(0.5f, 0.5f, 1.0f, 1.0f));
		}

		/*	Default samplers.	*/
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

		/*	Create all buffers.	*/
		{

			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize = 0;
			GLint maxUniformBlockBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockBufferSize);

			/*	*/
			this->UBOStructure.common_size_align = Math::align<size_t>(sizeof(GlobalSceneState), minMapBufferSize);
			this->UBOStructure.common_size_total_align = this->UBOStructure.common_size_align * BufferRoundRobinSize;
			this->UBOStructure.common_base_offset = 0;

			/*	*/
			this->UBOStructure.max_node_per_binding = 1024; // maxUniformBufferSize / sizeof(NodeData);
			const size_t max_nodes = 4096;
			const size_t NrNodes =
				max_nodes * sizeof(NodeData); // TODO: change number based on the max bininded uniform size.
			this->UBOStructure.node_size_align = Math::align<size_t>(NrNodes, minMapBufferSize);
			this->UBOStructure.node_size_total_align = this->UBOStructure.node_size_align * BufferRoundRobinSize;

			/*	*/
			const size_t max_bindable_materials = 4096;
			this->UBOStructure.material_align_size =
				Math::align<size_t>(max_bindable_materials * sizeof(MaterialData), minMapBufferSize);
			this->UBOStructure.material_align_total_size =
				this->UBOStructure.material_align_size * BufferRoundRobinSize;

			/*	*/
			this->UBOStructure.light_align_size = Math::align<size_t>(sizeof(LightData), minMapBufferSize);
			this->UBOStructure.light_align_total_size = this->UBOStructure.light_align_size * BufferRoundRobinSize;

			const size_t total_ubo_size =
				this->UBOStructure.node_size_total_align + this->UBOStructure.common_size_total_align +
				this->UBOStructure.material_align_total_size + this->UBOStructure.light_align_total_size;

			this->UBOStructure.node_base_offset = this->UBOStructure.common_size_total_align;
			this->UBOStructure.material_base_offset =
				this->UBOStructure.common_size_total_align + this->UBOStructure.node_size_total_align;
			this->UBOStructure.light_base_offset = this->UBOStructure.common_size_total_align +
												   this->UBOStructure.node_size_total_align +
												   this->UBOStructure.material_align_total_size;

			/*	*/
			glGenBuffers(1, &this->UBOStructure.node_and_common_uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.node_and_common_uniform_buffer);

			if (glBufferStorage) {

				/*	*/
				this->useCoherent = true;

				/*	Create and map buffer.	*/
				glBufferStorage(GL_UNIFORM_BUFFER, total_ubo_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
				uint8_t *pdata = (unsigned char *)glMapBufferRange(
					GL_UNIFORM_BUFFER, 0, total_ubo_size,
					GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT |
						GL_MAP_INVALIDATE_RANGE_BIT);

				/*	*/
				{
					// this->stageCommonBufferBase = (GlobalSceneState *)&pdata[0];
					for (size_t index = 0; index < stageCommonRobin.buffers.size(); index++) {

						this->UBOStructure.common_offsets[index] =
							this->UBOStructure.common_base_offset + this->UBOStructure.common_size_align * index;
						this->stageCommonRobin.buffers[index] =
							(GlobalSceneState *)&pdata[this->UBOStructure.common_size_align * index];

						*this->stageCommonRobin.buffers[index] = GlobalSceneState();
					}
				}

				/*	*/
				{
					uint8_t *baseNode = &pdata[this->UBOStructure.node_base_offset];

					for (size_t index = 0; index < stageNodeDataRobin.buffers.size(); index++) {

						this->UBOStructure.node_offsets[index] =
							this->UBOStructure.node_base_offset + this->UBOStructure.node_size_align * index;

						this->UBOStructure.node_prev_offsets[index] =
							this->UBOStructure.node_base_offset +
							this->UBOStructure.node_size_align * ((index + 2) % stageCommonRobin.buffers.size());

						this->stageNodeDataRobin.buffers[index] =
							(NodeData *)&baseNode[index * this->UBOStructure.node_size_align];
					}
				}

				/*	*/
				{
					uint8_t *baseMaterial = &pdata[this->UBOStructure.material_base_offset];

					for (size_t index = 0; index < stageCommonRobin.buffers.size(); index++) {

						this->UBOStructure.mateiral_offsets[index] =
							this->UBOStructure.material_base_offset + this->UBOStructure.material_align_size * index;
						this->stageMaterialDataRobin.buffers[index] =
							(MaterialData *)&baseMaterial[index * this->UBOStructure.material_align_size];
					}
				}

				/*	Setup Light Data Structure.	*/
				{
					uint8_t *baseLight = &pdata[this->UBOStructure.light_base_offset];

					for (size_t index = 0; index < stageLightData.buffers.size(); index++) {
						/*	*/
						this->UBOStructure.light_offsets[index] =
							this->UBOStructure.light_base_offset + this->UBOStructure.light_align_size * index;
						/*	*/
						this->stageLightData.buffers[index] =
							(LightData *)&baseLight[this->UBOStructure.light_align_size * index];
						*this->stageLightData.buffers[index] = LightData();
					}
				}

			} else {
				/*	*/

				glBufferData(GL_UNIFORM_BUFFER, total_ubo_size, nullptr, GL_DYNAMIC_DRAW);
				/*	TODO: create buffer on heap for staging.	*/
			}

			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
	}

	void Scene::update(const float deltaTime) {

		/*	*/
		this->timer.update(); // TODO: forward deltaTime
		this->stageCommonRobin.getBuffer(this->getRoundRobinIndex())->time[0] = this->timer.getElapsed<float>();
		this->stageCommonRobin.getBuffer(this->getRoundRobinIndex())->time[1] = deltaTime;

		/*	Update animations.	*/
		for (size_t anim_index = 0; anim_index < this->animations.size(); anim_index++) {
			/*	*/
			AnimationPlayer &animationClip = *this->getAnimation()[anim_index];
			// this->animations[x].curves;
		}

		/*	*/
		this->stageCommonRobin.buffers[getRoundRobinIndex()]->time[0] = deltaTime;
		this->updateBuffers();
	}

	void Scene::updateBuffers() {

		const size_t common_offset = this->UBOStructure.node_offsets[this->getRoundRobinIndex()];
		const size_t node_offset = this->UBOStructure.node_offsets[this->getRoundRobinIndex()];
		const size_t materail_offset = this->UBOStructure.mateiral_offsets[this->getRoundRobinIndex()];
		const size_t light_offset = this->UBOStructure.light_offsets[this->getRoundRobinIndex()];

		/*	Update global scene.	*/
		GlobalSceneState *globalSceneState = this->stageCommonRobin.buffers[getRoundRobinIndex()];
		globalSceneState->renderSettings.fogSettings = this->settings.fogSettings;
		globalSceneState->renderSettings.ambientColor = this->settings.ambientColor;

		/*	*/
		NodeData *baseNodeData = this->stageNodeDataRobin.buffers[getRoundRobinIndex()];
		size_t node_index = 0;
		auto copyQueue = renderQueue; // TODO: fix performance
		for (const Node *node : copyQueue) {
			baseNodeData[node_index++].model = node->getGlobalMatrix(); // modelGlobalTransform;
		}

		/*	Update Materials.	*/
		MaterialData *materialBase = this->stageMaterialDataRobin.buffers[this->getRoundRobinIndex()];
		size_t material_index = 0;
		for (; material_index < this->materials.size(); material_index++) {

			materialBase[material_index].info.x = material_index;
			materialBase[material_index].ambientColor = this->materials[material_index].ambient;
			materialBase[material_index].diffuseColor = this->materials[material_index].diffuse;
			materialBase[material_index].specular_roughness = glm::vec4(
				glm::vec3(this->materials[material_index].specular), this->materials[material_index].shinininess);
			materialBase[material_index].emission = this->materials[material_index].emission;
			materialBase[material_index].transparency = this->materials[material_index].transparent;
			materialBase[material_index].clip_[0] = this->materials[material_index].clipping;
			materialBase[material_index].clip_[1] = this->materials[material_index].bumpiness;
			materialBase[material_index].clip_[2] = this->materials[material_index].metalic;
		}

		/*	Update Lights.	*/
		LightData *stageLightBase = this->stageLightData.buffers[getRoundRobinIndex()];

		stageLightBase->directionalCount = 0;
		stageLightBase->pointCount = 0;
		size_t light_count = 0;
		for (light_count = 0; light_count < getLights().size(); light_count++) {
			Light *light = getLights()[light_count];

			switch (light->getLightType()) {

			case Light::LightType::Directional: {

				DirectionalLight *dirLight = dynamic_cast<DirectionalLight *>(light);
				DirectionalLightData *lightData = &stageLightBase->directional[stageLightBase->directionalCount];

				const glm::vec3 light_direction = dirLight->getDirectionalLight();

				lightData->lightColor = light->color;
				lightData->lightDirection = glm::vec4(light_direction, 1);

				/*	Shadow Setup.	*/
				lightData->lightShadow.shadow[0] = light->getShadowStrength();
				lightData->lightShadow.shadow[1] = light->bias;

				if (light->getShadowStrength() > 0) {

					const float near_plane = -(dirLight->getShadowDistance());
					const float far_plane = (dirLight->getShadowDistance());

					const glm::mat4 lightProjection = glm::ortho(
						-dirLight->getShadowDistance(), dirLight->getShadowDistance(), -dirLight->getShadowDistance(),
						dirLight->getShadowDistance(), near_plane, far_plane);

					const glm::mat4 lightView = glm::lookAt(
						dirLight->getPosition(), dirLight->getPosition() + light_direction * 100.0f, dirLight->up());
					const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

					light->shadowData.lightSpaceMatrix = lightSpaceMatrix;
					lightData->lightShadow.lightSpaceMatrix = lightSpaceMatrix;
				}

				stageLightBase->directionalCount++;
			} break;
			case Light::LightType::Point: {
				PointLight *pointLight = dynamic_cast<PointLight *>(light);
				PointLightInstance *lightData = &stageLightBase->pointLight[stageLightBase->pointCount];

				lightData->color = light->color;
				lightData->position = pointLight->getPosition();
				lightData->range = pointLight->getRange();

				if (light->shadow > 0) {
				}

				stageLightBase->pointCount++;
			} break;
			default:
			case Light::LightType::Spot:
				break;
			}
		}

		glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.node_and_common_uniform_buffer);

		/*	*/
		if (useCoherent) {

			/*	Update constant data.	*/
			glFlushMappedBufferRange(GL_UNIFORM_BUFFER, common_offset, this->UBOStructure.common_size_align);

			/*	Update Node Data.	*/
			glFlushMappedBufferRange(GL_UNIFORM_BUFFER, node_offset, node_index * sizeof(NodeData));

			/*	Update Material.	*/
			glFlushMappedBufferRange(GL_UNIFORM_BUFFER, materail_offset, material_index * sizeof(MaterialData));

			/*	Update Lights.	*/
			glFlushMappedBufferRange(GL_UNIFORM_BUFFER, light_offset, this->UBOStructure.light_align_size);

			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		} else {
			// TODO: add
		}
	}

	void Scene::culling(Frustum *frustum) {

		/*	*/
		this->visableNodes.clear();

		/*	Frustum Culling.	*/
		if (this->settings.frustumSettings.useFrustum && frustum) {

			/*	*/
			// TODO: multi thread
			const size_t num_threads = 6;
			static std::vector<std::vector<Node *>> objects(num_threads, std::vector<Node *>());
			for (size_t i = 0; i < objects.size(); i++) {
				objects[i].clear();
				objects[i].reserve(1024);
			}

#pragma omp parallel for collapse(1) num_threads(num_threads)
			for (size_t node_index = 0; node_index < this->getNodes().size(); node_index++) {

				Node *node = this->getNodes()[node_index];

				/*	Check if any of the meshes are visable. */
				for (size_t mesh_index = 0; mesh_index < node->geometryObjectIndex.size(); mesh_index++) {

					if (true) {

						const AABB aabb = fragcore::AABB::createMinMax(
							Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
							Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2]));

						const Vector3 sphere_world_position =
							(GLM2E<float, 4, 4>(node->modelGlobalTransform) *
							 Vector4(aabb.getCenter().x(), aabb.getCenter().y(), aabb.getCenter().z(), 1.0f))
								.head(3);

						BoundingSphere sphere = BoundingSphere(sphere_world_position, aabb.getHalfSize().norm());

						if (frustum->intersectionSphere(sphere) != Frustum::Out) {
							/*	*/
							objects[omp_get_thread_num()].push_back(node);
							break;
						}

					} else {

						/*	Compute world space AABB.	*/
						const AABB aabb = GeometryUtility::computeBoundingBox(
							fragcore::AABB::createMinMax(
								Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
								Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2])),
							GLM2E<float, 4, 4>(node->modelGlobalTransform));

						if (frustum->intersectionAABB(aabb) != Frustum::Out) {
							/*	*/
							objects[omp_get_thread_num()].push_back(node);
							break;
						}
					}
				}
			}

			for (size_t i = 0; i < objects.size(); i++) {
				this->visableNodes.insert(this->visableNodes.end(), objects[i].begin(), objects[i].end());
			}

		} else {
			this->visableNodes = this->getNodes();
		}
	}

	void Scene::render(Camera *camera) {
		/* Optionally populate */

		// TODO: fix camera argument.
		if (camera) {
			GlobalSceneState *globalScene = this->stageCommonRobin.buffers[getRoundRobinIndex()];
			globalScene->camera = *camera;
			CameraController *cameraController = dynamic_cast<CameraController *>(
				camera); // TODO: remove controller once the camera start using the base Node
			globalScene->camera = *cameraController;

			/*	*/
			globalScene->proj[0] = camera->getProjectionMatrix();
		}

		/*	*/
		this->culling(camera);

		/*	*/
		this->render();

		/*	*/
		if (useDebug()) {

			const bool renderWireframe = Math::isFlagSet<unsigned int>(debugMode, DebugMode::Wireframe);
			if (renderWireframe) {
				const std::string debugDomain = "Wireframe";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, debugDomain.size(), debugDomain.data());
				for (size_t i = 0; i < visableNodes.size(); i++) {

					/*	*/
					this->renderNode(visableNodes[i]);
				}

				glPopDebugGroup();
			}

			const bool renderBoundingShapes = Math::isFlagSet<unsigned int>(debugMode, DebugMode::BoundingBox);
			if (renderBoundingShapes) {
				const std::string debugDomain = "Debug";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, debugDomain.size(), debugDomain.data());
				/*	Populate.	*/

				const std::vector<RenderQueue> order = {RenderQueue::Background,  RenderQueue::Geometry,
														RenderQueue::AlphaTest,	  RenderQueue::GeometryLast,
														RenderQueue::Transparent, RenderQueue::Overlay};

				/*	*/
				for (auto it = order.begin(); it != order.end(); it++) {
					// RenderQueue renderQueue = (*it).first;
					std::deque<const Node *> nodeQueue = this->renderBucketQueue[(*it)];
					for (auto itQ = nodeQueue.begin(); itQ != nodeQueue.end(); itQ++) {

						const Node *node = (*itQ);
						// this->debugDrawer->addAABB(node->bound.aabb, glm::vec4(0.3f, 1.0f 0.3f, 1.0f));
					}
				}

				this->debugDrawer->draw(camera, nullptr);

				glPopDebugGroup();
			}
		}
	}

	void Scene::render(Light *light) {

		if (light) {
			GlobalSceneState *globalScene = this->stageCommonRobin.buffers[getRoundRobinIndex()];

			/*	*/
			globalScene->camera.far = light->getShadowDistance();
			globalScene->camera.near = 0;
			globalScene->camera.position = glm::vec4(light->getPosition(), 1.0f);
			globalScene->camera.proj = light->getProjectionMatrix();
			globalScene->camera.view = light->getViewMatrix();
			globalScene->camera.viewProj =
				light->shadowData.lightSpaceMatrix; // (globalScene->camera.proj * globalScene->camera.view);

			/*	*/
			globalScene->proj[0] = light->getProjectionMatrix();

			this->updateBuffers(); // TODO: update only camera
		}

		/*	*/
		this->culling(light);

		/*	*/
		this->render();
	}

	void Scene::render() {

		/*	Reset States.	*/
		this->currentNodeIndex = 0;
		this->currentBindedMaterial = nullptr;

		// TODO: sort materials and geometry.
		this->sortRenderQueue();

		// TODO: merge by shared geometries.

		/*	Iterate through each node.	*/

		// TODO: impl update bone data.
		/*	*/

		/*	Bind common data for all drawcall.	*/
		{
			const size_t common_offset = this->UBOStructure.common_offsets[this->getRoundRobinIndex()];
			const size_t light_offset = this->UBOStructure.light_offsets[this->getRoundRobinIndex()];

			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.common_buffer_binding,
							  this->UBOStructure.node_and_common_uniform_buffer, common_offset,
							  this->UBOStructure.common_size_align);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.light_buffer_binding,
							  this->UBOStructure.node_and_common_uniform_buffer, light_offset,
							  this->UBOStructure.light_align_total_size);
		}

		const std::vector<RenderQueue> order = {RenderQueue::Background,  RenderQueue::Geometry,
												RenderQueue::AlphaTest,	  RenderQueue::GeometryLast,
												RenderQueue::Transparent, RenderQueue::Overlay};

		/*	*/
		for (auto it = order.begin(); it != order.end(); it++) {
			// RenderQueue renderQueue = (*it).first;
			std::deque<const Node *> nodeQueue = this->renderBucketQueue[(*it)];

			/*	*/
			const std::string domain = fmt::format("{}", (int)(*it));

			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, domain.size(), domain.data());
			for (auto itQ = nodeQueue.begin(); itQ != nodeQueue.end(); itQ++) {
				const Node *node = (*itQ);
				this->renderNode(node);
			}
			glPopDebugGroup();
		}

		for (auto it = this->renderQueue.begin(); it != this->renderQueue.end(); it++) {
			const Node *node = (*it);
			// this->renderNode(node);
		}

		/*	*/
		if (this->debugMode & DebugMode::Wireframe) {
			/*	*/
			for (const Node *node : this->renderQueue) {
				/*	*/
				// const Node *node = this->renderQueue[x];
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

		this->renderPassFrameIndex++;
	}

	void Scene::bindTexture(const MaterialObject &material, const TextureTypeBinding texture_type) {

		/*	*/
		const unsigned int textureMapIndex = texture_type;
		unsigned int texture_id = 0;

		const int materialTextureIndex = material.texture_index[texture_type];

		const bool valid_texture_avaiable = materialTextureIndex >= 0 && materialTextureIndex < refTexture.size();
		if (valid_texture_avaiable) {

			const TextureAssetObject *tex = &this->refTexture[materialTextureIndex];

			/*	*/
			if (tex && tex->texture > 0) {
				texture_id = tex->texture;
			} else {
				texture_id = this->default_textures[texture_type];
			}

			const MaterialTextureSampling &sampling = material.texture_sampling[materialTextureIndex];

			/*	*/
			glSamplerParameteri(this->samplers[textureMapIndex], GL_TEXTURE_MIN_FILTER,
								sampling.filtering == TextureFilterMode::Nearset ? GL_NEAREST_MIPMAP_NEAREST
																				 : GL_LINEAR_MIPMAP_LINEAR);
			glSamplerParameteri(this->samplers[textureMapIndex], GL_TEXTURE_MAG_FILTER,
								sampling.filtering == TextureFilterMode::Nearset ? GL_NEAREST : GL_LINEAR);

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

		/*	*/
		glBindSampler(textureMapIndex, this->samplers[textureMapIndex]);
	}

	void Scene::bindMaterial(const MaterialObject *material) {

		/*	Only bind if different material the current material binded.	*/
		if (this->currentBindedMaterial != material) {

			this->bindTexture(*material, TextureTypeBinding::Diffuse);
			this->bindTexture(*material, TextureTypeBinding::Normal);
			this->bindTexture(*material, TextureTypeBinding::AlphaMask);
			this->bindTexture(*material, TextureTypeBinding::Emission);
			this->bindTexture(*material, TextureTypeBinding::AmbientOcclusion);
			this->bindTexture(*material, TextureTypeBinding::Displacement);
			this->bindTexture(*material, TextureTypeBinding::Specular_Roughness);
			this->bindTexture(*material, TextureTypeBinding::Metal);

			// this->bindTexture(material, TextureType::Irradiance); //TODO: enable once material has been binded
			// with irradiance texture

			this->bindTexture(*material, TextureTypeBinding::DepthBuffer);

			/*	*/
			const RenderQueue domain = getQueueDomain(*material);

			glDisable(GL_STENCIL_TEST);
			glDepthFunc(GL_LESS);

			/*	*/
			glPolygonMode(GL_FRONT_AND_BACK, material->wireframe_mode ? GL_LINE : GL_FILL);

			/*	*/
			if (domain == RenderQueue::Transparent) {
				/*	*/
				glEnable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				/*	*/
				material->blend_func_mode;
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
			if (material->culling_both_side_mode) {
				glDisable(GL_CULL_FACE);
				glCullFace(GL_FRONT_AND_BACK);
			} else {
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
			}

			/*	*/
			this->currentBindedMaterial = (MaterialObject *)material;
		}
	}

	void Scene::renderNode(const Node *node) {

		/*	Update binding offset.	*/
		if ((currentNodeIndex % this->UBOStructure.max_node_per_binding) == 0) {

			/*	*/
			const unsigned int node_block_offset_base = currentNodeIndex / this->UBOStructure.max_node_per_binding;
			const size_t node_total_offset = this->UBOStructure.node_offsets[this->getRoundRobinIndex()] +
											 (node_block_offset_base * 65536); // TODO:fix constants.

			const size_t node_prev_total_offset = this->UBOStructure.node_prev_offsets[this->getRoundRobinIndex()] +
												  (node_block_offset_base * 65536); // TODO:fix constants.

			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_buffer_binding,
							  this->UBOStructure.node_and_common_uniform_buffer, node_total_offset,
							  this->UBOStructure.node_size_align);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_prev_buffer_binding,
							  this->UBOStructure.node_and_common_uniform_buffer, node_prev_total_offset,
							  this->UBOStructure.node_size_align);

			/*	*/
			const size_t material_offset = this->UBOStructure.mateiral_offsets[this->getRoundRobinIndex()];
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.material_buffer_binding,
							  this->UBOStructure.node_and_common_uniform_buffer, material_offset,
							  this->UBOStructure.material_align_size);
		}

		/*	*/
		for (size_t geo_index = 0; geo_index < node->geometryObjectIndex.size(); geo_index++) {

			/*	Setup material.	*/
			const int material_index = node->materialIndex[geo_index];
			{

				const MaterialObject &material = this->materials[material_index];
				this->bindMaterial(&material);
			}

			const int mesh_index = node->geometryObjectIndex[geo_index];
			const MeshObject &refMesh = this->refGeometry[mesh_index];
			glBindVertexArray(refMesh.vao);

			/*	Material, model matrix.	*/
			glVertexAttribI2i(8, material_index, currentNodeIndex % this->UBOStructure.max_node_per_binding);
			/*	*/
			glDrawElementsBaseVertex(fragcore::GLHelper::getPrimitive(refMesh.primitiveType), refMesh.nrIndicesElements,
									 GL_UNSIGNED_INT, (void *)(sizeof(unsigned int) * refMesh.indices_offset),
									 refMesh.vertex_offset);

			// glBindVertexArray(0);
		}

		/*	Update internal states*/
		this->currentNodeIndex++;
	}

	void Scene::sortRenderQueue() {

		this->renderQueue.clear();
		this->renderBucketQueue.clear();

		/*	*/
		for (size_t x = 0; x < this->visableNodes.size(); x++) {

			/*	*/
			const Node *node = this->visableNodes[x];
			if (node->materialIndex.empty()) {
				continue;
			}

			/*	*/
			const bool validMaterialIndex = node->materialIndex[0] < this->materials.size();

			if (validMaterialIndex) {

				const MaterialObject *material = &this->materials[node->materialIndex[0]];
				assert(material);

				const RenderQueue domain = getQueueDomain(*material);

				renderBucketQueue[domain].push_back(node);

				if (domain >= RenderQueue::Transparent) {
					this->renderQueue.push_back(node);
				} else {
					this->renderQueue.push_front(node);
				}

			} else {
				std::cerr << "Invalid Material " << node->getName() << std::endl;
			}
		}

		/*	Sort Transparent Objects. Based on priority.	*/
		// std::sort(vec.begin(), vec.end(), [this, &index, &edges](const int_iter it1, const int_iter it2) -> bool {
		// 	index[it1 - int_vec.begin()] < index[it2 - int_vec.begin()];
		// });

		//				int priority = computeMaterialPriority(*material);

		renderBucketQueue[RenderQueue::Transparent];

		/*	Sort based on Shared mesh objects.	*/

		/*	Sort Transparent Objects. Based on distance.	*/
	}

	int Scene::computeMaterialPriority(const MaterialObject &material) const noexcept {
		const bool use_clipping = material.maskTextureIndex >= 0 && material.maskTextureIndex < refTexture.size();
		const bool useBlending = material.opacity < 1.0f;

		return (useBlending * 1000) + (use_clipping * 100);
	}

	RenderQueue Scene::getQueueDomain(const MaterialObject &material) const noexcept {
		const bool useGeometryAlpha = material.clipping < 1;
		const bool useBlending = material.transparent[3] < 1.0f ||
								 (material.texture_index[TextureTypeBinding::AlphaMask] >= 0 && !useGeometryAlpha);
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

	void Scene::renderUI() { this->settingUI.draw(); }

} // namespace glsample