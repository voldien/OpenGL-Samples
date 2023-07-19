#pragma once
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <glm/glm.hpp>
#define FLYTHROUGH_CAMERA_IMPLEMENTATION 1
#include "flythrough_camera.h"

class CameraController {
  public:
	CameraController() = default;

	void update(float delta) {

		const Uint8 *state = SDL_GetKeyboardState(nullptr);

		bool w = state[SDL_SCANCODE_W];
		bool a = state[SDL_SCANCODE_A];
		bool s = state[SDL_SCANCODE_S];
		bool d = state[SDL_SCANCODE_D];
		bool alt = state[SDL_SCANCODE_LALT];
		bool shift = state[SDL_SCANCODE_LSHIFT];
		// mouse movement.
		SDL_PumpEvents(); // make sure we have the latest mouse state.

		int buttons = SDL_GetMouseState(&x, &y);

		float xDiff = -(xprev - x) * xspeed;
		float yDiff = -(yprev - y) * yspeed;
		xprev = x;
		yprev = y;

		if (!enable_Navigation) {
			w = false;
			a = false;
			s = false;
			d = false;
		}

		float speed = 100.0f;
		if (shift) {
			speed *= 2.5f;
		}
		if (!alt) {
			flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], delta, speed,
									 0.5f * activated, fov, xDiff, yDiff, w, a, s, d, 0, 0, 0);
		}
	}
	void enableNavigation(bool enable) { this->enable_Navigation = enable; }

	const glm::mat4 &getViewMatrix() const noexcept { return this->view; }

	const glm::vec3 getPosition() const noexcept { return this->pos; }
	void setPosition(const glm::vec3 &position) noexcept { this->pos = position; }

	void lookAt(const glm::vec3 &position) noexcept { this->look = glm::normalize(position - this->getPosition()); }

	const glm::vec3 &getUp() const { return this->up; }
	float getFOV() const { return this->fov; }
	void setFOV(float FOV) noexcept { this->fov = FOV; }

	const glm::vec3 &getLookDirection() const noexcept { return this->look; }

  private:
	float fov = 80.0f;
	float speed = 100;
	float activated = 1.0f;
	float xspeed = 0.5f;
	float yspeed = 0.5f;

	bool enable_Navigation = true;

	int x, y, xprev, yprev;

	glm::mat4 view;
	glm::vec3 pos = {0.0f, 1.0f, 0.0f};
	glm::vec3 look = {0.0f, 0.0f, 1.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
};
