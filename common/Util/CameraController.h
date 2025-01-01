/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
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
#include <SDL2/SDL_keyboard.h>
#include <SDLInput.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class CameraController : public Frustum {
	  public:
		~CameraController() override = default;

		void update(const float deltaTime) noexcept;

		void enableNavigation(const bool enable) noexcept;
		void enableLook(const bool enable) noexcept;

		const glm::mat4 &getViewMatrix() const noexcept;
		const glm::mat4 getRotationMatrix() const noexcept;
		const glm::mat4 getViewTranslationMatrix() const noexcept;

		const glm::vec3 &getLookDirection() const noexcept;
		const glm::vec3 getPosition() const noexcept;
		void setPosition(const glm::vec3 &position) noexcept;

		const glm::vec3 &getUp() const noexcept;

		void lookAt(const glm::vec3 &position) noexcept;

		bool hasMoved() const noexcept;

	  protected:
		void update() noexcept;

		void updateFrustum();

	  private:
		float speed = 100;
		float activated = 1.0f;
		float xspeed = 0.5f;
		float yspeed = 0.5f;

		float fastSpeed = 2.5f;

		bool enabled_Navigation = true;
		bool enabled_Look = true;

		int x{}, y{}, xprev{}, yprev{};

		glm::mat4 view{};

		glm::vec3 pos = {0.0f, 1.0f, 0.0f};
		glm::vec3 look = {0.0f, 0.0f, 1.0f};
		glm::vec3 up = {0.0f, 1.0f, 0.0f};
	};

} // namespace glsample