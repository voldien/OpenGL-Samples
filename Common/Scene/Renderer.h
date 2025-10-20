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
#include "Math3D/AABB.h"
#include "Node.h"
#include "Scene/Material.h"

namespace glsample {

	/*	*/
	class Renderer : public Node {
	  public:
		Renderer() = default;

		enum class ShadowCastMode {
			Disabled,
			Enabled,
			TwoSided,
			ShadowOnly,
		};

		bool isVisable() const noexcept { return true; }
		bool receiveShadows() const noexcept { return true; }

		ShadowCastMode getShadowCastMode() noexcept { return ShadowCastMode::Enabled; }

	  public: /*	*/
		NodeType getNodeType() const noexcept override { return NodeType::Renderer; }

		const fragcore::AABB &getLocalBoundingBox() const noexcept { return this->boundingBox; }
		fragcore::AABB &getLocalBoundingBox() noexcept { return this->boundingBox; }

		fragcore::AABB getGlobalBoundingBox() const noexcept { return this->boundingBox; }

		 Material& getMaterial(const size_t index){return this->materials[index];}
		 MeshObject& getMesh(const size_t index){return this->meshes[index];}

		/*	Geometry and material.	*/
		std::vector<MeshObject &> meshes;
		std::vector<Material &> materials;

		std::vector<unsigned int> geometryObjectIndex;
		std::vector<unsigned int> materialIndex;

	  public:
		/*	*/
		fragcore::AABB boundingBox;
		fragcore::Bound bound{};
	};
} // namespace glsample
