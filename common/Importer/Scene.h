/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include <deque>

namespace glsample {

	enum TextureType : unsigned int {
		Diffuse = 0,	/*	*/
		Normal = 1,		/*	*/
		AlphaMask = 2,	/*	*/
		Specular = 3,	/*	*/
		Emission = 4,	/*	*/
		Reflection = 5, /*	*/
		Displacement,
	};

	enum class TexturePBRType : unsigned int {
		Albedo = 0,		  /*	*/
		Normal,			  /*	*/
		Metal,			  /*	*/
		Roughness,		  /*	*/
		AmbientOcclusion, /*	*/
		Displacement,	  /*	*/
	};

	/**
	 * @brief
	 *
	 */
	class Scene : public fragcore::UIDObject {
	  public:
		Scene() { this->init(); }
		virtual ~Scene();

		virtual void init();

		virtual void release();

		virtual void update(const float deltaTime);

		// virtual void updateBuffers();

		virtual void render(); // TODO, add camera.

		virtual void renderNode(const NodeObject *node);

		virtual void sortRenderQueue();

	  public:
		const std::vector<NodeObject *> &getNodes() const noexcept { return this->nodes; }

		const std::vector<MeshObject> &getMeshes() const noexcept { return this->refGeometry; }
		std::vector<MeshObject> &getMeshes() noexcept { return this->refGeometry; }

	  public:
		/*	TODO add queue structure.	*/
		std::deque<const NodeObject *> renderQueue;

		std::vector<NodeObject *> nodes;
		std::vector<MeshObject> refGeometry;
		std::vector<TextureAssetObject> refTexture;
		std::vector<MaterialObject> materials;
		std::vector<animation_object_t> animations;

	  protected: /*	Default texture if texture from material is missing.*/
		int normalDefault = -1;
		int diffuseDefault = -1;
		int roughnessSpecularDefault = -1;
		int emissionDefault = -1;

	  public:
		// Template

		template <typename T = Scene> static T loadFrom(ModelImporter &importer) {

			T scene;

			/*	*/
			scene.nodes = importer.getNodes();
			ImportHelper::loadModelBuffer(importer, scene.refGeometry);
			ImportHelper::loadTextures(importer, scene.refTexture);
			scene.materials = importer.getMaterials();

			return scene;
		}
		static void RenderUI(Scene &scene);
	};
} // namespace glsample