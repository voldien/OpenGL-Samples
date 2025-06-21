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
#include "Util/Frustum.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC Camera : public Frustum {

	  public:
		Camera() noexcept { this->updateProjectionMatrix(); }

		void calcFrustumPlanes(const Vector3 &position, const Vector3 &look_forward, const Vector3 &up,
							   const Vector3 &right) override {
			/*	*/
			const float halfVSide = this->getFar() * ::tanf(Math::degToRad(this->getFOVDegree()) * 0.5f);
			const float halfHSide = halfVSide * this->getAspect();

			/*	*/
			const Vector3 farDistance = this->getFar() * look_forward;

			/*	*/ // TODO: impl
			switch (getMode()) {
			case CameraMode::Orthographic:
			case CameraMode::Perspective:
				break;
			}
			this->planes[NEAR_PLANE] = {position + this->getNear() * look_forward, look_forward};
			this->planes[FAR_PLANE] = {position + farDistance, -look_forward};

			this->planes[RIGHT_PLANE] = {position, (farDistance - right * halfHSide).cross(up)};
			this->planes[LEFT_PLANE] = {position, up.cross(farDistance + right * halfHSide)};

			this->planes[TOP_PLANE] = {position, right.cross(farDistance - up * halfVSide)};
			this->planes[BOTTOM_PLANE] = {position, (farDistance + up * halfVSide).cross(right)};
		}

		void setAspect(const float aspect) noexcept {
			this->aspect = aspect;
			this->updateProjectionMatrix();
		}
		float getAspect() const noexcept { return this->aspect; }

		void setNear(const float near) noexcept {
			this->near = near;
			this->updateProjectionMatrix();
		}
		float getNear() const noexcept { return this->near; }

		void setFar(const float far) noexcept {
			this->far = far;
			this->updateProjectionMatrix();
		}
		float getFar() const noexcept { return this->far; }

		float getFOVDegree() const noexcept { return this->fov_degree; }
		void setFOVDegree(const float FOV_degree) noexcept {
			this->fov_degree = FOV_degree;
			this->updateProjectionMatrix();
		}

		const glm::mat4 &getProjectionMatrix() const noexcept { return this->proj; }

		// TODO: Refractor
		enum class CameraMode { Orthographic, Perspective };
		// TODO: Refractor
		void setMode(const CameraMode newMode) {
			this->mode = newMode;
			this->updateProjectionMatrix();
		}
		CameraMode getMode() const noexcept { return this->mode; }

	  protected:
		void updateProjectionMatrix() noexcept {
			switch (getMode()) {
			case CameraMode::Orthographic:
				this->proj = glm::ortho(this->left, this->right, this->bottom, this->top, -this->far, this->far);
				break;
			case CameraMode::Perspective:
			default:
				this->proj =
					glm::perspective(glm::radians(this->getFOVDegree() * 0.5f), this->aspect, this->near, this->far);
				break;
			}
		}

	  protected:
		float fov_degree = 80.0f;
		float aspect = 16.0f / 9.0f;
		float left = -10;
		float right = 10;
		float top = 10;
		float bottom = -10;
		float near = 0.45f;
		float far = 1650.0f;
		glm::mat4 proj{};
		CameraMode mode = CameraMode::Perspective;
	};
} // namespace glsample