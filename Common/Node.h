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

#include "Core/Object.h"
#include "DataStructure/ITree.h"
#include "Importer/ModelImporter.h"
#include "Scene/Transform.h"

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC Node : public glsample::TransformGLM,
							public fragcore::ITree<Node>,
							public fragcore::Object,
							public NodeObject /*	TODO: remove*/ {
	  public:
		Node() = default;
		Node(const NodeObject *node);
		~Node() override = default;

	  public:

		glm::vec3 getLocalPosition() const noexcept;
		glm::vec3 getLocalScale() const noexcept;
		glm::quat getLocalRotation() const noexcept;

		glm::mat4 getViewMatrix() const noexcept;
		glm::mat4 getRotationMatrix() const noexcept;
		glm::mat4 getViewTranslationMatrix() const noexcept;

	  public:
		/*	*/
		fragcore::Bound bound{};

		/*	Geometry and material.	*/
		std::vector<unsigned int> geometryObjectIndex;
		std::vector<unsigned int> materialIndex;
	};

} // namespace glsample
