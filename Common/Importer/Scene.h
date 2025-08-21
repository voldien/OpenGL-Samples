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
#include "Core/UIDObject.h"
#include "DataStructure/PoolAllocator.h"
#include "GLSampleSession.h"
#include "ModelImporter.h"
#include "SampleHelper.h"
#include <deque>

namespace glsample {

	class Light : public Node {
	  public:
		enum class LightType {
			Directional,
			Point,
		};
		LightType lightType;
		glm::vec4 color = glm::vec4(1);
	};

	class DirectionalLight : public Light {
	  public:
		DirectionalLight() { this->lightType = LightType::Directional; }
	};

	class PointLight : public Light {
	  public:
		PointLight() { this->lightType = LightType::Point; }

		float intensity = 1;
		float range = 5;
		float constant_attenuation = 1;
		float linear_attenuation = 0.1f;
		float quadratic_attenuation = 0.025f;
	};

	enum TextureType : unsigned int {
		Diffuse = 0,					 /*	*/
		Normal = 1,						 /*	*/
		AlphaMask = 2,					 /*	*/
		Specular_Roughness = 3,			 /*	*/
		Emission = 4,					 /*	*/
		Reflection = 5,					 /*	*/
		AmbientOcclusion = 6,			 /*	*/
		Displacement = 7,				 /*	*/
		Metal = 8,						 /*	*/
		Reserve1 = 9,					 /*	*/
		Irradiance = 10,				 /*	*/
		PreFilter = 11,					 /*	*/
		BRDFLUT = 12,					 /*	*/
		DepthBuffer = 13,				 /*	*/
		DirectionalLightDepthBuffer = 14 /*	*/
	};

	/*	The higher the later the will be rendered in the rendering queue.	*/
	enum RenderQueue : unsigned int {
		Background = 500,	 /*  */
		Geometry = 1000,	 /*  */
		AlphaTest = 1500,	 /*  */
		GeometryLast = 1600, /*  */
		Transparent = 2000,	 /*  */
		Overlay = 3000,		 /*  */
	};

	enum DebugMode : unsigned int {
		None = 0,
		Wireframe = 0x1,
		BoundingBox = 0x2,
	};

	class AnimationPlayer {
	  public:
		AnimationPlayer(AnimationObject &animation) {}

		float time{};
		unsigned int mode{};
		AnimationObject *animation{};
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

		virtual void init();

		virtual void release();

		virtual void update(const float deltaTime);

		virtual void updateBuffers();

		virtual void culling(Frustum *frustum); // TODO: add

		virtual void render(Camera *camera);
		virtual void render();

		virtual void bindMaterial(const MaterialObject *material);
		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

		virtual void renderUI();

	  public: /*	*/
			  //	void enableDebug();
	  public:
		const std::vector<NodeObject *> &getNodes() const noexcept { return this->nodes; }

		const std::vector<MeshObject> &getMeshes() const noexcept { return this->refGeometry; }
		std::vector<MeshObject> &getMeshes() noexcept { return this->refGeometry; }
		std::vector<MaterialObject> &getMaterials() noexcept { return this->materials; }
		std::vector<Light *> getLights() noexcept { return this->lights; }

		DirectionalLightData *getDirectionalLight(const size_t index = 0) noexcept {
			return &this->stageLightData.getBase()->directional[index];
		}

	  protected:
		void bindTexture(const MaterialObject &material, const TextureType texture_type);
		int computeMaterialPriority(const MaterialObject &material) const noexcept;
		RenderQueue getQueueDomain(const MaterialObject &material) const noexcept;
		size_t getRoundRobinIndex() const noexcept {
			return this->frameIndex % UniformDataStructure::bufferRoundRobinSize;
		}

	  protected:
		/*	*/
		MaterialObject *currentBindedMaterial = nullptr;

		/*	TODO add queue structure.	*/
		std::map<RenderQueue, std::deque<const NodeObject *>> renderBucketQueue;
		std::deque<const NodeObject *> renderQueue;
		std::vector<NodeObject *> visableNodes;

		std::vector<NodeObject *> nodes;
		std::vector<MeshObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<AnimationObject> animations;
		std::vector<Light *> lights;
		PoolAllocator<DirectionalLight> DirLightPool;
		PoolAllocator<PointLight> PointLightPool;

		using GlobalRenderSettings = struct _global_rendering_settings_t {
			glm::vec4 ambientColor = glm::vec4(1, 1, 1, 1);
			unsigned int IrradianceTexture = 0;
			FogSettings fogSettings;
			unsigned int FrustumCullingMode = 0;
		};

		using GlobalSceneState = struct common_constant_data_t {
			// TODO: keep multi frame camera frustum.
			CameraInstanceData camera;

			FrustumInstance frustum{};

			GlobalRenderSettings renderSettings = GlobalRenderSettings();

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

		using MaterialData = struct _material_data_t {
			glm::ivec4 info;				  /*	*/
			glm::vec4 ambientColor;			  /*	*/
			glm::vec4 diffuseColor;			  /*	*/
			glm::vec4 transparency;			  /*	*/
			glm::vec4 specular_roughness;	  /*	*/
			glm::vec4 emission;				  /*	*/
			glm::vec4 clip_ = glm::vec4(0.8); /*	*/
		};

		/*	*/
		bool useCoherent = true;

		GlobalSceneState *stageCommonBufferBase = nullptr;
		std::array<GlobalSceneState *, 3> stageCommonRobin;
		NodeData *stageNodeData = nullptr;
		NodeData *stagPrevNodeData = nullptr;
		MaterialData *stageMaterialData = nullptr;
		StageBuffer<LightData *, 3> stageLightData;

	  protected: /*	Default texture if texture from material is missing.*/
		std::array<unsigned int, 16> default_textures;
		std::array<unsigned int, 16> samplers;

		using Debug = struct debug_t {
			DebugMode debugMode;
		};

		using RenderingSettings = struct rendering_settings_t {
			DebugMode debugMode = DebugMode::None;
			bool frustumCulling = false;
		};

		RenderingSettings settings;

		DebugMode debugMode = DebugMode::None;
		bool frustumCulling = false;
		size_t currentNodeIndex = 0;

		template<unsigned int bufferRoundRobinSize>
		struct uniform_buffer_collection {
			unsigned int base_offset = 0;
			std::array<unsigned int, bufferRoundRobinSize> node_offsets;
			unsigned int size_align = 0;
			unsigned int size_total_align = 0;
			unsigned int max_item_per_binding = 0;
		};

		using UniformDataStructure = struct uniform_data_structure {
			/*	*/
			static const size_t bufferRoundRobinSize = 3;
			UBOObject uniform_buffer;
			unsigned int node_and_common_uniform_buffer; // TODO: removed and replace with uniform_buffer;

			unsigned int node_base_offset = 0;
			std::array<unsigned int, bufferRoundRobinSize> node_offsets;
			unsigned int node_size_align = 0;
			unsigned int node_size_total_align = 0;
			unsigned int max_node_per_binding = 0;

			unsigned int material_base_offset = 0;
			std::array<unsigned int, bufferRoundRobinSize> mateiral_offsets;
			unsigned int material_align_size = 0;
			unsigned int material_align_total_size = 0;
			unsigned int max_material_per_block = 0;

			unsigned int light_base_offset = 0;
			std::array<unsigned int, bufferRoundRobinSize> light_offsets;
			unsigned int light_align_size = 0;
			unsigned int light_align_total_size = 0;
			unsigned int max_light_per_binding = 0;

			unsigned int common_base_offset = 0;
			unsigned int common_size_align = 0;
			unsigned int common_size_total_align = 0;

			/*	Buffer Binding Values.	*/
			unsigned int binding_set = 0;
			unsigned int common_buffer_binding = 1;
			unsigned int node_buffer_binding = 2;
			unsigned int bone_buffer_binding = 3;
			unsigned int material_buffer_binding = 4;
			unsigned int light_buffer_binding = 5;
		};

		UniformDataStructure UBOStructure;

		unsigned int frameIndex = 0;
		static const unsigned int frameChainCount = 3;
	};
} // namespace glsample
