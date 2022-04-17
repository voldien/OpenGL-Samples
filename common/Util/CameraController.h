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
		const Uint8 *state = SDL_GetKeyboardState(NULL);
		bool w = state[SDLK_w];
		bool a = state[SDLK_a];
		bool s = state[SDLK_s];
		bool d = state[SDLK_d];

		// mouse movement.
		SDL_PumpEvents(); // make sure we have the latest mouse state.

		int buttons = SDL_GetMouseState(&x, &y);

		float xDiff = -(xprev - x) * xspeed;
		float yDiff = (yprev - y) * yspeed;
		xprev = x;
		yprev = y;
		flythrough_camera_update(&pos[0], &look[0], &up[0], &view[0][0], delta, 100.0f * 1, 0.5f * activated, fov,
								 xDiff, yDiff, w, a, s, d, 0, 0, 0);
	}
	const glm::mat4 &getViewMatrix() const noexcept { return this->view; }

  private:
	float fov = 80.0f;
	float speed = 100;
	float activated = 1.0f;
	float xspeed = 0.5f;
	float yspeed = 0.5f;

	int x, y, xprev, yprev;

	glm::mat4 view;
	glm::vec3 pos = {0.0f, 0.0f, 0.0f};
	glm::vec3 look = {0.0f, 0.0f, 1.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
};
