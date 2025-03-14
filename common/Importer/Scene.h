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
#include "Core/UIDObject.h"
#include "GLSampleSession.h"
#include "ImportHelper.h"
#include "ModelImporter.h"
#include "SampleHelper.h"
#include <deque>

namespace glsample {

	enum TextureType : unsigned int {
		Diffuse = 0,		  /*	*/
		Normal = 1,			  /*	*/
		AlphaMask = 2,		  /*	*/
		Specular = 3,		  /*	*/
		Emission = 4,		  /*	*/
		Reflection = 5,		  /*	*/
		AmbientOcclusion = 6, /*	*/
		Displacement = 7,	  /*	*/
		Reserve0 = 8,		  /*	*/
		Reserve1 = 9,		  /*	*/
		Irradiance = 10,	  /*	*/
		PreFilter = 11,		  /*	*/
		BRDFLUT = 12,		  /*	*/
		DepthBuffer = 13,	  /*	*/
	};

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
	};
	class AnimationPlayer {
	  public:
		AnimationPlayer(AnimationObject &animation) {}

		float time{};
		unsigned int mode{};
		AnimationObject *animation;
	};

	/**
	 * @brief
	 *
	 */
	class Scene : public fragcore::UIDObject {
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

		virtual void bindMaterial(const MaterialObject* material);
		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

		virtual void renderUI();

	  public: /*	*/
			  //	void enableDebug();
	  public:
		const std::vector<NodeObject *> &getNodes() const noexcept { return this->nodes; }

		const std::vector<MeshObject> &getMeshes() const noexcept { return this->refGeometry; }
		std::vector<MeshObject> &getMeshes() noexcept { return this->refGeometry; }

	  protected:
		void bindTexture(const MaterialObject &material, const TextureType texture_type);
		int computeMaterialPriority(const MaterialObject &material) const noexcept;
		RenderQueue getQueueDomain(const MaterialObject &material) const noexcept;

	  protected:
		using GlobalRenderSettings = struct alignas(16) _global_rendering_settings_t {
			glm::vec4 ambientColor = glm::vec4(1, 1, 1, 1);
			unsigned int IrradianceTexture = 0;
			FogSettings fogSettings;
			unsigned int FrustumCullingMode = 0;
		};
		using CommonConstantData = struct alignas(16) common_constant_data_t {
			// TODO: keep multi frame camera frustum.
			CameraInstance camera;
			FrustumInstance frustum{};

			GlobalRenderSettings renderSettings = GlobalRenderSettings();

			/*	*/
			glm::mat4 proj[3]{};
			glm::mat4 view[3]{};

			glm::vec4 time; /*	elapsed, delta,	*/
		};
		using NodeData = struct alignas(16) _node_data_t {
			glm::mat4 model;
		};
		using LightData = struct alignas(16) _light_data_t {
			DirectionalLight directional[16];
			PointLightInstance pointLight[64];
			unsigned int directionalCount = 0;
			unsigned int pointCount = 0;
		};
		using MaterialData = struct alignas(16) _material_data_t {
			glm::vec4 ambientColor;
			glm::vec4 diffuseColor;
			glm::vec4 transparency;
			glm::vec4 specular_roughness;
			glm::vec4 emission;
			glm::vec4 clip_ = glm::vec4(0.8);
		};

		/*	*/
		CommonConstantData *stageCommonBuffer = nullptr;
		NodeData *stageNodeData = nullptr;
		MaterialData *stageMaterialData = nullptr;
		LightData *lightData = nullptr;

		MaterialObject* currentBindedMaterial = nullptr;

		/*	TODO add queue structure.	*/
		std::map<RenderQueue, std::deque<const NodeObject *>> renderBucketQueue;
		std::deque<const NodeObject *> renderQueue;
		std::vector<NodeObject *> visableNodes;

		std::vector<NodeObject *> nodes;
		std::vector<MeshObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<AnimationObject> animations;

	  protected: /*	Default texture if texture from material is missing.*/
		std::array<unsigned int, 16> default_textures;
		std::array<unsigned int, 16> samplers;

		DebugMode debugMode = DebugMode::None;
		bool frustumCulling = false;
		size_t currentNodeIndex = 0;

		using UniformDataStructure = struct uniform_data_structure {
			/*	*/
			static const size_t nrUniformBuffer = 3;
			unsigned int node_and_common_uniform_buffer;

			unsigned int node_offset = 0;
			unsigned int node_size_align = 0;
			unsigned int node_size_total_align = 0;
			unsigned int max_node_per_binding = 0;

			unsigned int material_offset = 0;
			unsigned int material_align_size = 0;
			unsigned int material_align_total_size = 0;
			unsigned int max_material_per_block = 0;

			unsigned int light_offset = 0;
			unsigned int light_align_size = 0;
			unsigned int light_align_total_size = 0;

			unsigned int common_offset = 0;
			unsigned int common_size_align = 0;
			unsigned int common_size_total_align = 0;

			/*	*/
			unsigned int common_buffer_binding = 1;
			unsigned int node_buffer_binding = 2;
			unsigned int bone_buffer_binding = 3;
			unsigned int material_buffer_binding = 4;
			unsigned int light_buffer_binding = 5;
		};

		UniformDataStructure UBOStructure;

		int frameIndex = 0;
		static const unsigned int frameChainCount = 3;

	  public:
		template <typename T = Scene> static T loadFrom(ModelImporter &importer) {
			T scene;

			/*	*/
			scene.nodes = importer.getNodes();
			ImportHelper::loadModelBuffer(importer, scene.refGeometry);
			ImportHelper::loadTextures(importer, scene.refTexture);
			scene.materials = importer.getMaterials();

			return scene;
		}
	};
} // namespace glsample