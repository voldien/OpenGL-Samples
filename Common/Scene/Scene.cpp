#include "Scene.h"
#include "../Common.h"
#include "Core/SystemInfo.h"
#include "IO/IFileSystem.h"
#include "Importer/ModelImporter.h"
#include "Math/Bitwise.h"
#include "Math3D/Color.h"
#include "RenderDesc.h"
#include "SampleHelper.h"
#include "Scene/Frustum.h"
#include "Scene/RenderQueue.h"
#include "Util/DebugDrawer.h"
#include "Util/GLDebugDrawer.h"
#include <GL/glew.h>
#include <GLHelper.h>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <list>
#include <omp.h>
#include <ostream>
#include <set>
#include <sys/types.h>

namespace glsample {

	Scene::Scene() : settingUI(*this) { this->timer.start(); }

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

		/*	Release uniform buffers*/

		/*	*/
		for (size_t tex_index = 0; tex_index < this->refTexture.size(); tex_index++) {
			if (glIsTexture(this->refTexture[tex_index].texture)) {
				glDeleteTextures(1, &this->refTexture[tex_index].texture);
			}
		}
		/*	Release internal default textures.	*/
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
			this->debugDrawer = new GLDebugDrawManager(filesystem);
		}
		/*	Create Internal shaders.	*/

		/*	*/
		// this->DirLightPool.resize(64);
		// this->PointLightPool.resize(64);
		this->visableNodes.reserve(4096);

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
			this->UBOStructure.common_size_align = Math::align<size_t>(sizeof(GlobalSceneStateData), minMapBufferSize);
			this->UBOStructure.common_size_total_align = this->UBOStructure.common_size_align * BufferRoundRobinSize;
			this->UBOStructure.common_base_offset = 0;

			/*	*/
			this->UBOStructure.max_node_per_binding = maxUniformBlockBufferSize / sizeof(NodeData);
			const size_t max_node_count_in_buffer = static_cast<long>(4096) * 2;
			const size_t nodeDataInBytes =
				max_node_count_in_buffer *
				sizeof(NodeData); // TODO: change number based on the max bininded uniform size.
			this->UBOStructure.node_size_align = Math::align<size_t>(nodeDataInBytes, minMapBufferSize);
			this->UBOStructure.node_size_total_align = this->UBOStructure.node_size_align * BufferRoundRobinSize;

			/*	*/
			const size_t max_bindable_materials = maxUniformBlockBufferSize / sizeof(MaterialData);
			this->UBOStructure.max_material_per_block = max_bindable_materials;
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
			glGenBuffers(1, &this->UBOStructure.shared_uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.shared_uniform_buffer);

			if (glBufferStorage) {

				/*	*/
				this->useCoherent = true;

				/*	Create and map buffer.	*/
				glBufferStorage(GL_UNIFORM_BUFFER, total_ubo_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
				uint8_t *pdata =
					(unsigned char *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, total_ubo_size,
													  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
														  GL_MAP_FLUSH_EXPLICIT_BIT); // GL_MAP_UNSYNCHRONIZED_BIT |

				/*	*/
				{
					// this->stageCommonBufferBase = (GlobalSceneState *)&pdata[0];
					for (size_t index = 0; index < stageCameraCommonRobin.buffers.size(); index++) {

						this->UBOStructure.common_offsets[index] =
							this->UBOStructure.common_base_offset + this->UBOStructure.common_size_align * index;
						this->stageCameraCommonRobin.buffers[index] =
							(GlobalSceneStateData *)&pdata[this->UBOStructure.common_size_align * index];

						*this->stageCameraCommonRobin.buffers[index] = GlobalSceneStateData();
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
							this->UBOStructure.node_size_align * ((index + 2) % stageCameraCommonRobin.buffers.size());

						this->stageNodeDataRobin.buffers[index] =
							(NodeData *)&baseNode[index * this->UBOStructure.node_size_align];
					}
				}

				/*	*/
				{
					uint8_t *baseMaterial = &pdata[this->UBOStructure.material_base_offset];

					for (size_t index = 0; index < stageCameraCommonRobin.buffers.size(); index++) {

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

		// this->rootNode = (this->nodePool.back());
	}

	void Scene::update(const float deltaTime) {

		/*	Update Time */
		this->timer.update();
		this->stageCameraCommonRobin.getBuffer(this->getRoundRobinIndex())->time[0] += deltaTime;
		this->stageCameraCommonRobin.getBuffer(this->getRoundRobinIndex())->time[1] = deltaTime;

		/*	Update animations.	*/
		for (size_t anim_index = 0; anim_index < this->animations.size(); anim_index++) {
			/*	*/
			AnimationPlayer &animationClip = *this->getAnimation()[anim_index];
			// this->animations[x].curves;
		}

		/*	*/
		this->updateBuffers();
	}

	void Scene::updateBuffers() {

		const size_t common_offset = this->UBOStructure.node_offsets[this->getRoundRobinIndex()];
		const size_t node_offset = this->UBOStructure.node_offsets[this->getRoundRobinIndex()];
		const size_t materail_offset = this->UBOStructure.mateiral_offsets[this->getRoundRobinIndex()];
		const size_t light_offset = this->UBOStructure.light_offsets[this->getRoundRobinIndex()];

		/*	Update global scene.	*/
		GlobalSceneStateData *globalSceneState = this->stageCameraCommonRobin.buffers[getRoundRobinIndex()];
		globalSceneState->renderSettings.ambientColor = this->settings.lightSettings.ambientColor;
		globalSceneState->renderSettings.specularColor = this->settings.lightSettings.specularColor;

		/*	*/
		NodeData *baseNodeData = this->stageNodeDataRobin.buffers[getRoundRobinIndex()];
		size_t node_index = 0;
		for (size_t i = 0; i < renderQueue.size(); i++) {
			auto copyQueue = renderQueue; // TODO: fix performance
			for (const Node *node : copyQueue[i]) {
				baseNodeData[node_index++].model = node->getGlobalMatrix();
			}
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
			materialBase[material_index].clip_[0] = materialBase[material_index].transparency.a < 1
														? 0
														: this->materials[material_index].getGraphicSettings().clipping;
			materialBase[material_index].clip_[1] = this->materials[material_index].bumpiness;
			materialBase[material_index].clip_[2] = this->materials[material_index].metalic;
		}

		/*	Update Lights.	*/
		{
			LightData *stageLightBase = this->stageLightData.buffers[getRoundRobinIndex()];

			stageLightBase->directionalCount = 0;
			stageLightBase->pointCount = 0;
			size_t light_count = 0;
			for (light_count = 0; light_count < getLights().size(); light_count++) {
				Light *light = getLights()[light_count];

				/*	*/
				if (!light->isActive()) {
					continue;
				}

				switch (light->getLightType()) {

				case Light::LightType::Directional: {

					DirectionalLight *dirLight = dynamic_cast<DirectionalLight *>(light);
					DirectionalLightData *lightData = &stageLightBase->directional[stageLightBase->directionalCount];

					const glm::vec3 light_direction = dirLight->getDirectionalLight();

					lightData->lightColor = light->color;
					lightData->lightDirection = glm::vec4(light_direction, 1);

					/*	Shadow Setup.	*/
					lightData->lightShadow.shadow[0] = light->hasShadow() ? light->getShadowStrength() : 0.0f;
					lightData->lightShadow.shadow[1] = light->bias;
					lightData->lightShadow.shadow[2] = light->getShadowFade();

					if (light->getShadowStrength() > 0) {

						/*	*/
						const float near_plane = -(dirLight->getShadowDistance());
						const float far_plane = (dirLight->getShadowDistance());

						const glm::mat4 lightProjection = glm::ortho(
							-dirLight->getShadowDistance(), dirLight->getShadowDistance(),
							-dirLight->getShadowDistance(), dirLight->getShadowDistance(), near_plane, far_plane);

						/*	*/
						const glm::vec3 dir_position = this->getActiveCamera()
														   ? glm::ceil(this->getActiveCamera()->getPosition())
														   : dirLight->getPosition();
						const glm::mat4 lightView =
							glm::lookAt(dir_position, dir_position + light_direction * 100.0f, dirLight->up());
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
		}

		glBindBuffer(GL_UNIFORM_BUFFER, this->UBOStructure.shared_uniform_buffer);

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

	void Scene::culling(const Frustum *frustum) {

		/*	*/
		this->visableNodes.clear();

		static std::vector<Node *> activeNodes;
		activeNodes.clear();

		{

			std::function<void(const Node *, std::vector<Node *> &)> addIfActive;

			/*	*/
			addIfActive = [&addIfActive](const Node *root, std::vector<Node *> &active) {
				for (size_t i = 0; i < root->getNumChildren(); i++) {
					Node *node = root->getChild(i)->ptr();
					if (node->isActive()) {
						active.push_back(node);
						addIfActive(node, activeNodes);
					}
				}
			};

			/*	Extract Active Nodes.	*/
			Node *root = getRootNode();
			if (root->isActive()) {
				addIfActive(root, activeNodes);
			}
		}

		/*	Frustum Culling.	*/
		if (this->settings.frustumSettings.useFrustum && frustum) {

			/*	*/
			const static size_t num_threads = Math::max<size_t>(SystemInfo::getCPUCoreCount() / 4, 1);
			// TODO: make a single array.
			static std::vector<std::vector<Node *>> objects(num_threads, std::vector<Node *>());
			for (size_t i = 0; i < objects.size(); i++) {
				objects[i].clear();
				objects[i].reserve(1024);
			}

#pragma omp parallel for num_threads(num_threads) default(shared) schedule(static, 16)
			for (size_t node_index = 0; node_index < activeNodes.size(); node_index++) {

				Node *node = activeNodes[node_index];

				for (size_t mesh_index = 0; mesh_index < node->geometryObjectIndex.size(); mesh_index++) {
				}

				/*	Check if any of the meshes are visable. */
				for (size_t mesh_index = 0; mesh_index < Math::clamp<size_t>(node->geometryObjectIndex.size(), 0, 1);
					 mesh_index++) {

					/*	*/
					switch (this->settings.frustumSettings.FrustumCullingMode) {
					case FrustumCullingMode::BoundingSphere: {

						/*	*/
						const AABB aabb = fragcore::AABB::createMinMax(
							Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
							Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2]));

						const glm::vec4 sphere_world_position =
							node->getGlobalMatrix() * glm::vec4(aabb.getCenter(), 1);

						const BoundingSphere sphere =
							BoundingSphere(sphere_world_position, glm::length(aabb.getHalfSize()));

						if (frustum->intersectionSphere(sphere) != Frustum::Out) {
							/*	*/
							objects[omp_get_thread_num()].push_back(node);
							break;
						}

					} break;
					case FrustumCullingMode::BoundingBoxAABB: {

						/*	Compute world space AABB.	*/
						const AABB aabb = GeometryUtility::computeBoundingBox(
							fragcore::AABB::createMinMax(
								Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
								Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2])),
							node->modelGlobalTransform);

						/*	*/
						if (frustum->intersectionAABB(aabb) != Frustum::Out) {
							/*	*/
							objects[omp_get_thread_num()].push_back(node);
							break;
						}
					} break;
					default:
						break;
					}
				}
			}

			/*	*/
			#pragma omp single
			for (size_t i = 0; i < objects.size(); i++) {
				this->visableNodes.insert(this->visableNodes.end(), objects[i].begin(), objects[i].end());
			}

		} else {

			/*	Remove if disabled.	*/
			this->visableNodes = activeNodes;
		}
	}

	void Scene::render(Camera *camera, FrameBuffer *framebuffer) {

		/* Optionally populate */

		// TODO: fix camera argument.
		if (camera) {
			this->currentActiveCamera = camera;

			GlobalSceneStateData *globalScene = this->stageCameraCommonRobin.buffers[getRoundRobinIndex()];
			globalScene->camera = *camera;

			/*	*/
			globalScene->proj[0] = camera->getProjectionMatrix();
		}
		/*	*/
		this->culling(camera);

		// TODO: sort materials and geometry.
		this->sortRenderQueue();

		this->updateBuffers();

		/*	*/
		if (framebuffer) {
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);
			glViewport(0, 0, framebuffer->attachmentSize[0].x, framebuffer->attachmentSize[0].y);
		}

		/*	*/
		if (getRenderingSettings().preDepthRenderingSettings) {
			/*	*/
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			this->render();
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		this->getRenderingSettings().skybox.render(*camera);

		/*	*/
		this->render();

		/*	*/
		if (useDebug()) {

			const bool renderWireframe = Bitwise::isFlagSet<unsigned int>(debugMode, DebugMode::Wireframe);
			if (renderWireframe) {
				const std::string debugDomain = "Wireframe";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, debugDomain.size(), debugDomain.data());
				for (size_t i = 0; i < visableNodes.size(); i++) {

					/*	*/
					this->renderNode(visableNodes[i]);
				}

				glPopDebugGroup();
			}

			const bool renderBoundingShapes = Bitwise::isFlagSet<unsigned int>(debugMode, DebugMode::BoundingBox);
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
					std::deque<const Node *> nodeQueue = this->renderQueueDomainBucket[(*it)];
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

	void Scene::render(const Light *light) {

		if (!light) {
			return;
		}

		GlobalSceneStateData *globalScene = this->stageCameraCommonRobin.buffers[getRoundRobinIndex()];

		/*	*/
		globalScene->camera.far = light->getShadowDistance();
		globalScene->camera.near = 0;
		globalScene->camera.position = glm::vec4(light->getPosition(), 1.0f);
		globalScene->camera.proj = light->getProjectionMatrix();
		globalScene->camera.view = light->getViewMatrix();
		globalScene->camera.viewProj = light->shadowData.lightSpaceMatrix;

		/*	*/
		globalScene->proj[0] = light->getProjectionMatrix();

		/*	*/
		this->culling(light);

		// TODO: sort materials and geometry.
		this->sortRenderQueue();

		this->updateBuffers(); // TODO: update only camera

		/*	*/
		this->render();
	}

	void Scene::render(FrameBuffer *framebuffer) {

		/*	Reset States.	*/
		this->currentNodeIndex = 0;
		this->currentBindedMaterial = nullptr;

		// TODO: merge by shared geometries.

		/*	Iterate through each node.	*/

		// TODO: impl update bone data.
		/*	*/

		/*	Bind common data for all drawcall.	*/
		{
			const size_t common_offset = this->UBOStructure.common_offsets[this->getRoundRobinIndex()];
			const size_t light_offset = this->UBOStructure.light_offsets[this->getRoundRobinIndex()];

			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.common_buffer_binding,
							  this->UBOStructure.shared_uniform_buffer, common_offset,
							  this->UBOStructure.common_size_align);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.light_buffer_binding,
							  this->UBOStructure.shared_uniform_buffer, light_offset,
							  this->UBOStructure.light_align_total_size);
		}

		const std::vector<RenderQueue> order = {RenderQueue::Background,  RenderQueue::Geometry,
												RenderQueue::AlphaTest,	  RenderQueue::GeometryLast,
												RenderQueue::Transparent, RenderQueue::Overlay};

		/*	*/

		std::list<std::deque<const Node *>> lists;
		for (auto it = renderQueueDomainBucket.begin(); it != renderQueueDomainBucket.end(); it++) {
			lists.push_back((*it).second);
		}

		unsigned int domainIndex = 0;
		for (auto *it = this->renderQueue.begin(); it != renderQueue.end(); it++) {
			// RenderQueue renderQueue = (*it).first;
			/*	Makes a copy, to prevent the original gettinged emptied.	*/
			std::deque<const Node *> nodeQueue = (*it);

			/*	*/
			const std::string domain = fmt::format("{} {}", getRenderQueueSymbols()[domainIndex], (domainIndex));
			domainIndex++;

			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, domain.size(), domain.data());
			for (auto itQ = nodeQueue.begin(); itQ != nodeQueue.end(); itQ++) {
				const Node *node = (*itQ);

				/*	Ignore if disabled.	*/
				// TODO: remove, make it the frustum culling to decide this.
				if (!node->isActive()) {
					continue;
				}

				this->renderNode(node);
			}
			glPopDebugGroup();
		}

		for (auto *it = this->renderQueue.begin(); it != this->renderQueue.end(); it++) {
			// const Node *node = (*it);
			//  this->renderNode(node);
		}

		/*	*/
		if (this->debugMode & DebugMode::Wireframe) {
			/*	*/
			// for (const Node *node : this->renderQueue) {
			/*	*/
			// const Node *node = this->renderQueue[x];
			// this->renderNode(node);
			//}
		}

		/*	Reset some OpenGL States.	*/ // TODO: remove once
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL);
		glCullFace(GL_BACK);

		this->renderPassFrameIndex++;
	}

	void Scene::bindTexture(const Material &material, const TextureTypeBinding texture_type) {

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

	void Scene::bindMaterial(const Material *material) {

		/*	Only bind if different material the current material binded.	*/
		if (this->currentBindedMaterial != material) {

			// TODO: change to use
			if (material->program > 0) {
				glUseProgram(material->program);
			}

			this->bindMaterialTextures(material);

			// this->bindTexture(material, TextureType::Irradiance); //TODO: enable once material has been binded
			// with irradiance texture

			this->bindTexture(*material, TextureTypeBinding::DepthBuffer);

			/*	*/
			const RenderQueue domain = material->getGraphicSettings().queue;

			glDisable(GL_STENCIL_TEST);
			glDepthFunc(GL_LESS);

			/*	*/
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			/*	*/
			glDepthMask(material->getGraphicSettings().DepthWrite ? GL_TRUE : GL_FALSE);
			if (material->getGraphicSettings().DepthFunc != DepthFunc::NoCompare) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			/*	*/
			if (domain >= RenderQueue::Transparent) {
				/*	*/
				glEnable(GL_BLEND);
				/*	*/
				// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

				/*	*/
				switch (material->getGraphicSettings().blend_color_func) {
				case fragcore::BlendFunc::Zero:
				case fragcore::BlendFunc::One:
				case fragcore::BlendFunc::SrcColor:
				case fragcore::BlendFunc::OneMinusSrcColor:
				case fragcore::BlendFunc::SrcAlpha:
				case fragcore::BlendFunc::OneMinusSrcAlpha:
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case fragcore::BlendFunc::ConstantAlpha:
					break;
				default:
					break;
				}

				/*	*/
				switch (material->getGraphicSettings().blend_equ) {
				case fragcore::BlendEqu::NoEqu:
				case fragcore::BlendEqu::Addition:
					glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
					break;
				case fragcore::BlendEqu::Subtract:
					glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_SUBTRACT);
					break;
				case fragcore::BlendEqu::ReverseSubtract:
				case fragcore::BlendEqu::Min:
					glBlendEquationSeparate(GL_MIN, GL_MIN);
					break;
				case fragcore::BlendEqu::Max:
					glBlendEquationSeparate(GL_MAX, GL_MAX);
					break;
				default:
					break;
				}

			} else {
				/*	*/
				glDisable(GL_BLEND);
			}

			/*	Culling Mode.	*/
			switch (material->getGraphicSettings().cullingMode) {
			case fragcore::CullingMode::Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case fragcore::CullingMode::Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			case fragcore::CullingMode::FrontAndBack:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT_AND_BACK);
				break;
			case fragcore::CullingMode::None:
				glDisable(GL_CULL_FACE);
				glCullFace(GL_FRONT_AND_BACK);
				break;
			}

			/*	*/
			this->currentBindedMaterial = (Material *)material;
		}
	}

	void Scene::bindMaterialTextures(const Material *material) {

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
		// this->bindTexture(*material, TextureTypeBinding::Reflection);

		// glActiveTexture(GL_TEXTURE0 + TextureTypeBinding::Reflection);
		// glBindTexture(GL_TEXTURE_2D, this->getRenderingSettings().skybox.getTexture()); // TODO: relocate

		this->bindTexture(*material, TextureTypeBinding::DepthBuffer);
	}

	void Scene::renderNode(const Node *node) {

		/*	Update binding offset.	*/
		if ((currentNodeIndex % this->UBOStructure.max_node_per_binding) == 0) {

			/*	*/
			const size_t node_block_offset_base = currentNodeIndex / this->UBOStructure.max_node_per_binding;

			const size_t node_total_offset =
				this->UBOStructure.node_offsets[this->getRoundRobinIndex()] +
				(node_block_offset_base * this->UBOStructure.max_node_per_binding * sizeof(NodeData));

			const size_t node_prev_total_offset =
				this->UBOStructure.node_prev_offsets[this->getRoundRobinIndex()] +
				(node_block_offset_base * this->UBOStructure.max_node_per_binding * sizeof(NodeData));

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_buffer_binding,
							  this->UBOStructure.shared_uniform_buffer, node_total_offset,
							  this->UBOStructure.node_size_align);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.node_prev_buffer_binding,
							  this->UBOStructure.shared_uniform_buffer, node_prev_total_offset,
							  this->UBOStructure.node_size_align);

			/*	*/
			const size_t material_offset = this->UBOStructure.mateiral_offsets[this->getRoundRobinIndex()];
			glBindBufferRange(GL_UNIFORM_BUFFER, this->UBOStructure.material_buffer_binding,
							  this->UBOStructure.shared_uniform_buffer, material_offset,
							  this->UBOStructure.material_align_size);
		}

		/*	*/
		for (size_t geo_index = 0; geo_index < node->geometryObjectIndex.size(); geo_index++) {

			/*	Setup material.	*/
			const int material_index = node->materialIndex[geo_index];
			const Material &material = this->materials[material_index];
			this->bindMaterial(&material);

			/*	Setup Material.	*/
			const int mesh_index = node->geometryObjectIndex[geo_index];
			const MeshObject &refMesh = this->refGeometry[mesh_index];
			glBindVertexArray(refMesh.vao);

			/*	Material index, model matrix index.	*/
			glVertexAttribI2i(8, material_index, currentNodeIndex % this->UBOStructure.max_node_per_binding);

			/*	*/
			const size_t nrInstances = 1;
			if (this->getRenderingSettings().enabledTessellation && material.isTessellationEnabled()) {

				/*	*/
				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElementsInstancedBaseVertex(
					fragcore::GLHelper::getPrimitive(Primitive::Patchs), refMesh.nrIndicesElements, GL_UNSIGNED_INT,
					(void *)(sizeof(unsigned int) * refMesh.indices_offset), nrInstances, refMesh.vertex_offset);
			} else {

				/*	*/
				glDrawElementsInstancedBaseVertex(
					fragcore::GLHelper::getPrimitive(refMesh.primitiveType), refMesh.nrIndicesElements, GL_UNSIGNED_INT,
					(void *)(sizeof(unsigned int) * refMesh.indices_offset), nrInstances, refMesh.vertex_offset);
			}
		}

		/*	Update internal states*/
		this->currentNodeIndex++;
	}

	void Scene::sortRenderQueue() {

		/*	*/
		for (size_t i = 0; i < this->renderQueue.size(); i++) {
			this->renderQueue[i].clear();
		}
		this->renderQueueDomainBucket.clear();

		std::vector<Node *> visableNode2Sort = this->visableNodes;

		/*	*/ // TODO: sort by node tree.
		std::map<size_t, Node *> materialBuckets;
		std::map<size_t, std::set<const Material *>> materialsInDomain;
		for (size_t x = 0; x < visableNode2Sort.size(); x++) {

			/*	*/
			const Node *node = visableNode2Sort[x];

			/*	Exclude if no material associated.	*/
			if (node->materialIndex.empty()) {
				continue;
			}

			for (size_t material_index = 0; material_index < node->materialIndex.size(); material_index++) {

				/*	*/
				const size_t material_pool_index = node->materialIndex[material_index];
				const bool validMaterialIndex = material_pool_index < this->getMaterials().size();

				if (validMaterialIndex) {

					const Material *material = &this->getMaterials()[material_pool_index];

					assert(material);

					/*	TODO domain clamping.	*/
					const RenderQueue domain = material->getRenderQueue();

					materialsInDomain[domain].insert(material);
					this->renderQueueDomainBucket[domain].push_back(node);

				} else {
					std::cerr << "Invalid Material " << node->getName() << std::endl;
				}
			}
		}

		// multi thread.
		// #pragma omp parallel for
		for (size_t domain_index = 0; domain_index < getQueueTypesOrdered().size(); domain_index++) {
			const RenderQueue domain = getQueueTypesOrdered()[domain_index];

			std::deque<const Node *> queue = this->renderQueueDomainBucket[domain];

			if (this->getRenderingSettings().sortSharedMaterials) {

				std::sort(queue.begin(), queue.end(), [&](const Node *a, const Node *b) {
					return getMaterials()[a->materialIndex[0]].getUID() > getMaterials()[b->materialIndex[0]].getUID();
				});
			}

			this->renderQueue[domain_index] = queue;
		}

		if (this->getRenderingSettings().mergeInstances) {
			/*	Sort based on Shared mesh objects.	*/
		}

		if (this->getRenderingSettings().sortDistance) {
			/*	Sort Opque Objects. Based on distance.	*/
			/*	Sort Transparent Objects. Based on distance.	*/
		}
	}

	void Scene::renderUI() { this->settingUI.draw(); }

} // namespace glsample