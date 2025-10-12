/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "Common.h"
#include "Core/Time.h"
#include "Core/UIDObject.h"
#include "IO/IFileSystem.h"
#include "Importer/ModelImporter.h" //TODO: evntually remove.
#include "SampleHelper.h"
#include "Scene/Animation.h"
#include "Scene/Light.h"
#include "Scene/Material.h"
#include "Scene/RenderQueue.h"
#include "Scene/SceneSettingComponentUI.h"
#include "Skybox.h"
#include "Util/DebugDrawer.h"
#include <deque>

namespace glsample {

	enum class FrustumCullingMode {
		None,
		BoundingBoxAABB,
		BoundingSphere,
		MaxCullingMode,
	};

	enum TextureTypeBinding : unsigned int {
		Diffuse = 0,					 /*	*/
		Normal = 1,						 /*	*/
		AlphaMask = 2,					 /*	*/
		Specular_Roughness = 3,			 /*	*/
		Emission = 4,					 /*	*/
		Reflection = 5,					 /*	*/
		AmbientOcclusion = 6,			 /*	*/
		Displacement = 7,				 /*	*/
		Metal = 8,						 /*	*/
		BackBuffer = 9,					 /*	*/
		Irradiance = 10,				 /*	*/
		PreFilter = 11,					 /*	*/
		BRDFLUT = 12,					 /*	*/
		DepthBuffer = 13,				 /*	*/
		DirectionalLightDepthBuffer = 24 /*	*/
	};

	enum DebugMode : unsigned int {
		None = 0,
		Wireframe = 0x1,
		BoundingBox = 0x2,
	};

	/**
	 * @brief
	 *
	 */
	class Scene : public fragcore::UIDObject {
		friend class SceneHelper;

	  public:
		Scene();
		virtual ~Scene();

	  public:												  /*	*/
		virtual void init(IFileSystem *filesystem = nullptr); // TODO: add sample base reference for shared resources.

		virtual void release();

		virtual void update(const float deltaTime);

		virtual void render(Camera *camera, FrameBuffer *framebuffer = nullptr);
		virtual void render(const Light *light);
		virtual void render(FrameBuffer *framebuffer = nullptr); // Additional parameters.

		virtual void renderUI();

	  protected: /*	Internal backend methods.	*/
		virtual void
		bindMaterial(const Material *material); /*	TODO: Pass additional formation for determine what to bind.	*/
		virtual void bindMaterialTextures(const Material *material);

		virtual void renderNode(const Node *node);

		virtual void sortRenderQueue(); /*	TODO: add options.	*/

		virtual void updateBuffers(); // TODO: add specific regions.

		virtual void culling(const Frustum *frustum); // TODO: add, options

		bool useDebug() const noexcept { return this->debugMode > DebugMode::None; }

	  public: /*	Access Methods.	*/
		const Node *getRootNode() const noexcept { return this->rootNode; }
		Node *getRootNode() noexcept { return this->rootNode; }

		const std::vector<Node *> &getNodes() const noexcept { return this->nodes; }
		std::vector<Node *> &getNodes() noexcept { return this->nodes; }

		const std::vector<MeshObject> &getMeshes() const noexcept { return this->refGeometry; }
		std::vector<MeshObject> &getMeshes() noexcept { return this->refGeometry; }

		std::vector<Material> &getMaterials() noexcept { return this->materials; }
		const std::vector<Material> &getMaterials() const noexcept { return this->materials; }

		std::vector<Light *> &getLights() noexcept { return this->lights; }
		const std::vector<Light *> &getLights() const noexcept { return this->lights; }

		const std::vector<AnimationPlayer *> &getAnimation() const noexcept { return this->animations; }
		std::vector<AnimationPlayer *> &getAnimation() noexcept { return this->animations; }

		DirectionalLightData *getDirectionalLight(const size_t index = 0) noexcept {
			return &this->stageLightData.getBase()->directional[index];
		}
		const DirectionalLightData *getDirectionalLight(const size_t index = 0) const noexcept {
			return &this->stageLightData.getBase()->directional[index];
		}

		Camera *getActiveCamera() const noexcept { return this->currentActiveCamera; }
		Camera *getCamera(const unsigned int index) const noexcept { return cameras.at(index); }

		const std::vector<Camera *> &getCameras() const noexcept { return cameras; }
		std::vector<Camera *> &getCameras() noexcept { return cameras; }

	  protected:
		virtual void bindTexture(const Material &material, const TextureTypeBinding texture_type);
		size_t getRoundRobinIndex() const noexcept { return this->renderPassFrameIndex % BufferRoundRobinSize; }

	  protected:
		DebugDrawManager *debugDrawer = nullptr;

		/*	*/
		Material *currentBindedMaterial = nullptr;
		Camera *currentActiveCamera = nullptr;

		using RenderBatch = struct render_batch_t {
			MeshObject *mesh;
			unsigned int instances;
			Node *begin;
			RenderQueue queue;
		};

		using RenderPiplineSorter = struct render_pipeline_sorter_t {
			std::map<Material *, std::vector<RenderBatch>> renderBatches;
			std::map<RenderQueue, std::deque<const Node *>> renderQueueDomainBucket;
			std::array<std::deque<const Node *>, 6> renderQueue;
		};

		/*	TODO add queue structure.	*/
		// TODO: add batch struct.
		RenderPiplineSorter renderPipelineSorter;
		std::map<RenderQueue, std::deque<const Node *>> renderQueueDomainBucket;

		/*	*/
		std::array<std::deque<const Node *>, 6> renderQueue;
		std::vector<Node *> visableNodes;

		Node *rootNode = nullptr;
		std::vector<Node *> nodes;
		std::vector<Node> nodePool;
		std::vector<MeshObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<Material> materials;
		std::vector<AnimationPlayer *> animations;
		std::vector<Light *> lights;
		std::vector<Camera *> cameras;

		/*	*/
		std::map<size_t, glm::mat4> worldMatricesCache;
		std::map<size_t, AABB> worldAABBCache;

		// fragcore::PoolAllocator<DirectionalLight> DirLightPool;
		// fragcore::PoolAllocator<PointLight> PointLightPool;
		// fragcore::PoolAllocator<Node> nodePoolS;
		// fragcore::PoolAllocator<AnimationPlayer> animationss;

		using FrustumSettings = struct _frustum_settings_t {
			bool useFrustum;
			FrustumCullingMode FrustumCullingMode = FrustumCullingMode::BoundingBoxAABB;
		};

		unsigned int IrradianceTexture = 0;
		using GlobalRenderSettingData = struct _global_rendering_settings_t {
			glm::vec4 ambientColor = glm::vec4(1, 1, 1, 1);
			// ImageBasedLightningSettings imageBasedLightningSettings;
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
		};

		using GlobalSceneStateData = struct common_constant_data_t {
			// TODO: keep multi frame camera frustum.
			CameraInstanceData camera;
			FrustumInstance frustum{};
			GlobalRenderSettingData renderSettings = GlobalRenderSettingData();

			/*	*/
			glm::mat4 proj[3]{};
			glm::mat4 view[3]{};

			glm::vec4 time; /*	elapsed, delta,	*/
		};

		using NodeData = struct _node_data_t {
			glm::mat4 model;
		};

		using LightData = struct _light_data_t {
			DirectionalLightData directional[16];
			PointLightInstance pointLight[64];
			unsigned int directionalCount = 0;
			unsigned int pointCount = 0;
		};

		// TODO: replace with structure data.
		using MaterialData = struct _material_data_t {
			glm::ivec4 info;				  /*	*/
			glm::vec4 ambientColor;			  /*	*/
			glm::vec4 diffuseColor;			  /*	*/
			glm::vec4 transparency;			  /*	*/
			glm::vec4 specular_roughness;	  /*	*/
			glm::vec4 emission;				  /*	*/
			glm::vec4 clip_ = glm::vec4(0.8); /*	*/
		};

	  protected: /*	Default texture if texture from material is missing.*/
		std::array<unsigned int, 16> default_textures = {0};
		std::array<unsigned int, 16> samplers = {0};

		using RenderingStatistic = struct rendering_statistic_t {
			unsigned int bindedMaterials = 0;
			unsigned int bindedTextures = 0;
			unsigned int bindedSamplers = 0;
			unsigned int bindedUniforms = 0;
			unsigned int bindedGeometry = 0;
			unsigned int bindedPrograms = 0;
		};

		/*	*/
		using DebugSettings = struct debug_t {
			DebugMode debugMode;
			enum class CryptoMatte {
				Material,
				Object,
			};
			RenderingStatistic statistic;
		};

		using ImageBasedLightningSettings = struct image_based_lightning_settings_t {
			float intensity = 1;
			/*	Type of */
		};

		using LightSettings = struct light_settings_t {
			glm::vec4 ambientColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
		};

		using OcclusionAccelleration = struct occlusion_accelleration_t {};

		using OcclusionSettings = struct occlusion_settings_t {};

		using PreDepthRenderingSettings = struct pre_depth_rendering_settings : fragcore::Property<bool, size_t> {};

		using RenderingSettings = struct rendering_settings_t {
			bool enabledTessellation = false; // TODO: make to struct with more parameters.
			FrustumSettings frustumSettings;
			LightSettings lightSettings;
			PreDepthRenderingSettings preDepthRenderingSettings;
			Skybox skybox;
			// Render queue settings
			bool sortDistance = true;
			bool mergeInstances = true;
			bool sortSharedMaterials = true;
		};

		fragcore::Time timer;
		SceneSettingsUI settingUI;
		DebugMode debugMode = DebugMode::None;
		bool frustumCulling = false;
		size_t currentNodeIndex = 0;
		RenderingSettings settings;

		/*	*/
		bool useCoherent = true;

		static constexpr unsigned int BufferRoundRobinSize = 3;
		StageBuffer<GlobalSceneStateData *, BufferRoundRobinSize> stageCameraCommonRobin;
		StageBuffer<GlobalSceneStateData *, BufferRoundRobinSize> stageLightCommonRobin;
		StageBuffer<NodeData *, BufferRoundRobinSize> stageNodeDataRobin;
		StageBuffer<NodeData *, BufferRoundRobinSize> stagePreNodeDataRobin;
		StageBuffer<MaterialData *, BufferRoundRobinSize> stageMaterialDataRobin;
		StageBuffer<LightData *, BufferRoundRobinSize> stageLightData;

		template <unsigned int bufferRoundRobinSize> struct uniform_buffer_collection {
			unsigned int base_offset = 0;
			std::array<unsigned int, bufferRoundRobinSize> node_offsets;
			unsigned int size_align = 0;
			unsigned int size_total_align = 0;
			unsigned int max_item_per_binding = 0;
		};

		UBOPool UniformPool;

		using UniformDataStructure = struct uniform_data_structure {
			/*	*/

			UBOObject uniform_buffer{};

			unsigned int shared_uniform_buffer{}; // TODO: removed and replace with uniform_buffer;

			unsigned int node_base_offset = 0;
			std::array<unsigned int, BufferRoundRobinSize> node_offsets{};
			std::array<unsigned int, BufferRoundRobinSize> node_prev_offsets{};
			unsigned int node_size_align = 0;
			unsigned int node_size_total_align = 0;
			unsigned int max_node_per_binding = 0;

			unsigned int material_base_offset = 0;
			std::array<unsigned int, BufferRoundRobinSize> mateiral_offsets{};
			unsigned int material_align_size = 0;
			unsigned int material_align_total_size = 0;
			unsigned int max_material_per_block = 0;

			unsigned int light_base_offset = 0;
			std::array<unsigned int, BufferRoundRobinSize> light_offsets{};
			unsigned int light_align_size = 0;
			unsigned int light_align_total_size = 0;
			unsigned int max_light_per_binding = 0;

			std::array<unsigned int, BufferRoundRobinSize> common_offsets{};
			unsigned int common_base_offset = 0;
			unsigned int common_size_align = 0;
			unsigned int common_size_total_align = 0;

			/*	Buffer Binding Values.	*/
			unsigned int binding_set = 0;
			unsigned int common_buffer_binding = 1;
			unsigned int node_buffer_binding = 2;
			unsigned int node_prev_buffer_binding = 6;
			unsigned int bone_buffer_binding = 3;
			unsigned int bone_prev_buffer_binding = 7;
			unsigned int material_buffer_binding = 4;
			unsigned int light_buffer_binding = 5;
		};

		UniformDataStructure UBOStructure;

		unsigned int renderPassFrameIndex = 0;
		static const unsigned int frameChainCount = 3;

	  public:
		std::array<unsigned int, 16> &getSamplers() noexcept { return this->samplers; }

		const RenderingSettings &getRenderingSettings() const noexcept { return this->settings; }
		RenderingSettings &getRenderingSettings() noexcept { return this->settings; }

		DebugMode getDebugMode() const noexcept { return this->debugMode; }
		DebugMode &getDebugMode() noexcept { return this->debugMode; }

		std::vector<TextureAssetObject> &getTextures() noexcept { return this->refTexture; }
		DebugDrawManager *getDebugDrawer() noexcept { return this->debugDrawer; }
	};
} // namespace glsample
