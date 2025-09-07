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
#include "Transform.h"

namespace glsample {

	enum class NodeType { Node, Frustum, Camera, Renderer, Light, MaxNodeTypes };

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC Node : public glsample::TransformGLM, public fragcore::ITree<Node>, public fragcore::Object {
	  public:
		Node() = default;
		~Node() override = default;

	  public:
		void setPosition(const glm::vec3 &position) noexcept;
		glm::vec3 getPosition() noexcept;
		const glm::vec3 &getPosition() const noexcept;

		void setScale(const glm::vec3 &scale) noexcept;
		glm::vec3 getScale() const noexcept;

		const glm::quat &getRotation() const noexcept;
		void setRotation(const glm::quat &quat) noexcept;

		/*	*/
		Node *parent() const noexcept;

		glm::vec3 getLocalPosition() const noexcept;
		glm::vec3 getLocalScale() const noexcept;
		glm::quat getLocalRotation() const noexcept;

		void setLocalPosition(const glm::vec3 &localPosition) noexcept;
		void setLocalScale(const glm::vec3 &localScale) noexcept;
		void setLocalRotation(const glm::quat &localRotation) noexcept;

		glm::mat4 getGlobalMatrix() const noexcept;
		glm::mat4 getLocalMatrix() const noexcept;

		glm::mat4 getViewMatrix() const noexcept;
		glm::mat4 getLocalViewMatrix() const noexcept;
		glm::mat4 getRotationMatrix() const noexcept;
		glm::mat4 getViewTranslationMatrix() const noexcept;

	  public:
		/*	*/
		fragcore::Bound bound{};

		/*	Geometry and material.	*/
		std::vector<unsigned int> geometryObjectIndex;
		std::vector<unsigned int> materialIndex;

		/*	*/
		glm::mat4 modelGlobalTransform{};
		glm::mat4 modelLocalTransform{};
	};

} // namespace glsample
