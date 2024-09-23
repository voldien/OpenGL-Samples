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
	template <typename T> class Camera {
		static_assert(std::is_floating_point<T>::value, "Must be a decimal type(float/double/half).");

	  public:
		Camera() noexcept { this->updateProjectionMatrix(); }

		void setAspect(const T aspect) noexcept {
			this->aspect = aspect;
			this->updateProjectionMatrix();
		}
		T getAspect() const noexcept { return this->aspect; }

		void setNear(const T near) noexcept {
			this->near = near;
			this->updateProjectionMatrix();
		}
		T getNear() const noexcept { return this->near; }

		void setFar(const T far) noexcept {
			this->far = far;
			this->updateProjectionMatrix();
		}
		T getFar() const noexcept { return this->far; }

		T getFOV() const noexcept { return this->fov_degree; }
		void setFOV(const T FOV_degree) noexcept {
			this->fov_degree = FOV_degree;
			this->updateProjectionMatrix();
		}

		const glm::mat4 &getProjectionMatrix() const noexcept { return this->proj; }

	  protected:
		void updateProjectionMatrix() noexcept {
			this->proj = glm::perspective(glm::radians(this->getFOV() * static_cast<T>(0.5)), this->aspect, this->near,
										  this->far);
		}

	  protected:
		T fov_degree = 80.0f;
		T aspect = 16.0f / 9.0f;
		T near = 0.15f;
		T far = 1500.0f;
		glm::mat4 proj;
	};
} // namespace glsample