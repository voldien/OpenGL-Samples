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
#include "Scene/Frustum.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glsample {

	/**
	 * @brief
	 */
	class FVDECLSPEC Camera : public Frustum {
	  public:
		enum ClearMode {
			DontClear = (1 << 0), /*	*/
			Clear = (1 << 1),	  /*	*/
			SkyBox = (1 << 2)	  /*	*/
		};

		enum class CameraProjectionMode { Orthographic, Perspective, EquirecTangular };

	  public:
		Camera() noexcept;

		void calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
							   const glm::vec3 &right) noexcept override;

		void setAspect(const float aspect) noexcept;
		float getAspect() const noexcept;

		void setNear(const float near) noexcept;
		float getNear() const noexcept;

		void setFar(const float far) noexcept;
		float getFar() const noexcept;

		float getFOVDegree() const noexcept;
		void setFOVDegree(const float FOV_degree) noexcept;

		void setOrth(const float left, const float right, const float bottom, const float top, const float near,
					 const float far) noexcept;

		const glm::mat4 &getProjectionMatrix() const noexcept;
		glm::mat4 getProjectionMatrix() noexcept;

		void setProjectionMode(const CameraProjectionMode newMode);
		CameraProjectionMode getProjectionMode() const noexcept;

		/*	*/
	  public:
		NodeType getNodeType() const noexcept override { return NodeType::Camera; }

	  protected:
		void updateProjectionMatrix() noexcept;

	  protected:
		float fov_degree = 80.0f;
		float aspect = 16.0f / 9.0f;
		float left = -10;
		float right = 10;
		float top = 10;
		float bottom = -10;
		float near = 0.45f;
		float far = 2550.0f;
		glm::mat4 proj{};
		CameraProjectionMode mode = CameraProjectionMode::Perspective;
	};
} // namespace glsample
