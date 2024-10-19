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
#include "Math3D/Math3D.h"
#include "Util/Frustum.h"
#include "flythrough_camera.h"
#include <Input.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDLInput.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class CameraController : public Frustum {
	  public:
		CameraController() = default;

		void update(const float deltaTime) noexcept {

			const Uint8 *state = SDL_GetKeyboardState(nullptr);

			bool w = state[SDL_SCANCODE_W];
			bool a = state[SDL_SCANCODE_A];
			bool s = state[SDL_SCANCODE_S];
			bool d = state[SDL_SCANCODE_D];
			const bool alt = state[SDL_SCANCODE_LALT];
			const bool shift = state[SDL_SCANCODE_LSHIFT];
			const bool space = state[SDL_SCANCODE_SPACE];

			// mouse movement.
			SDL_PumpEvents(); // make sure we have the latest mouse state.

			SDL_GetMouseState(&x, &y);

			float xDiff = 0;
			float yDiff = 0;

			if (this->enabled_Look) {
				xDiff = -(xprev - x) * this->xspeed;
				yDiff = -(yprev - y) * this->yspeed;
				xprev = x;
				yprev = y;
			}

			/*	*/
			if (!this->enabled_Navigation) {
				w = false;
				a = false;
				s = false;
				d = false;
			}

			/*	*/
			float current_speed = this->speed;
			if (shift) {
				current_speed *= 2.5f;
			}

			/*	*/
			if (!alt && (this->enabled_Look || this->enabled_Navigation)) {
				flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], deltaTime,
										 current_speed, 0.5f * activated, this->fov_degree, xDiff, yDiff, w, a, s, d, 0,
										 0, 0);

				/*	*/
				this->updateFrustum();
			}
		}

		void enableNavigation(const bool enable) noexcept { this->enabled_Navigation = enable; }
		void enableLook(const bool enable) noexcept { this->enabled_Look = enable; }

		const glm::mat4 &getViewMatrix() const noexcept { return this->view; }
		const glm::mat4 getRotationMatrix() const noexcept {
			glm::quat rotation =
				glm::quatLookAt(glm::normalize(this->getLookDirection()), glm::normalize(this->getUp()));
			return glm::toMat4(rotation);
		}
		const glm::mat4 getViewTranslationMatrix() const noexcept {
			return glm::translate(glm::mat4(1), -this->getPosition());
		}

		const glm::vec3 &getLookDirection() const noexcept { return this->look; }
		const glm::vec3 getPosition() const noexcept { return this->pos; }
		void setPosition(const glm::vec3 &position) noexcept {
			this->pos = position;
			this->update();
			this->updateFrustum();
		}

		const glm::vec3 &getUp() const noexcept { return this->up; }

		void lookAt(const glm::vec3 &position) noexcept {
			this->look = glm::normalize(position - this->getPosition());
			this->update();
			this->updateFrustum();
		}

		bool hasMoved() const noexcept { return true; }

	  protected:
		void update() noexcept {
			flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], 0, 0,
									 0.5f * activated, this->fov_degree, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}

		void updateFrustum() {
			/*	*/
			const Vector3 position = Vector3(this->pos[0], this->pos[1], this->pos[2]);
			const Vector3 look = Vector3(this->look[0], this->look[1], this->look[2]).normalized();
			const Vector3 up = Vector3(this->up[0], this->up[1], this->up[2]).normalized();
			const Vector3 right = look.cross(up).normalized();

			this->calcFrustumPlanes(position, look, up, right);
		}

	  private:
		float speed = 100;
		float activated = 1.0f;
		float xspeed = 0.5f;
		float yspeed = 0.5f;

		float fastSpeed = 2.5f;

		bool enabled_Navigation = true;
		bool enabled_Look = true;

		int x, y, xprev, yprev;

		glm::mat4 view;

		glm::vec3 pos = {0.0f, 1.0f, 0.0f};
		glm::vec3 look = {0.0f, 0.0f, 1.0f};
		glm::vec3 up = {0.0f, 1.0f, 0.0f};
	};

} // namespace glsample