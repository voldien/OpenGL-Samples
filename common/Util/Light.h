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
	class FVDECLSPEC Light : public Frustum {
		static_assert(std::is_floating_point<float>::value, "Must be a decimal type(float/double/half).");

	  public:
		Light() noexcept { this->updateProjectionMatrix(); }

		void calcFrustumPlanes(const Vector3 &position, const Vector3 &look_forward, const Vector3 &up,
							   const Vector3 &right) override {}

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

		const glm::mat4 &getProjectionMatrix() const noexcept { return this->proj; }

	  protected:
		void updateProjectionMatrix() noexcept {}

	  protected:
		glm::mat4 proj{};
		float near = 0.45f;
		float far = 1650.0f;
	};
} // namespace glsample