#pragma once
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#define FLYTHROUGH_CAMERA_IMPLEMENTATION 1
#include "flythrough_camera.h"
#include <glm/gtc/matrix_transform.hpp>

class Camera {
  public:
};

class CameraController : public Camera {
  public:
	CameraController() = default;

	void update(const float deltaTime) noexcept {

		const Uint8 *state = SDL_GetKeyboardState(nullptr);

		bool w = state[SDL_SCANCODE_W];
		bool a = state[SDL_SCANCODE_A];
		bool s = state[SDL_SCANCODE_S];
		bool d = state[SDL_SCANCODE_D];
		bool alt = state[SDL_SCANCODE_LALT];
		bool shift = state[SDL_SCANCODE_LSHIFT];
		// mouse movement.
		SDL_PumpEvents(); // make sure we have the latest mouse state.

		const int buttons = SDL_GetMouseState(&x, &y);

		const float xDiff = -(xprev - x) * xspeed;
		const float yDiff = -(yprev - y) * yspeed;
		xprev = x;
		yprev = y;

		/*	*/
		if (!enable_Navigation) {
			w = false;
			a = false;
			s = false;
			d = false;
		}

		/*	*/
		float speed = this->speed;

		/*	*/
		if (shift) {
			speed *= 2.5f;
		}

		/*	*/
		if (!alt) {
			flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], deltaTime, speed,
									 0.5f * activated, fov, xDiff, yDiff, w, a, s, d, 0, 0, 0);
		}

		this->updateProjectionMatrix();
	}
	void enableNavigation(bool enable) { this->enable_Navigation = enable; }

	const glm::mat4 &getViewMatrix() const noexcept { return this->view; }
	const glm::mat4 getRotationMatrix() const noexcept {
		return glm::orientation(glm::normalize(this->getLookDirection()), this->getUp());
	}

	const glm::vec3 &getLookDirection() const noexcept { return this->look; }
	const glm::vec3 getPosition() const noexcept { return this->pos; }
	void setPosition(const glm::vec3 &position) noexcept { this->pos = position; }

	const glm::vec3 &getUp() const { return this->up; }

	void lookAt(const glm::vec3 &position) noexcept { this->look = glm::normalize(position - this->getPosition()); }

	void setAspect(const float aspect) noexcept { this->updateProjectionMatrix(); }
	float getAspect() const { return this->aspect; }

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

	float getFOV() const noexcept { return this->fov; }
	void setFOV(float FOV) noexcept {
		this->fov = FOV;
		this->updateProjectionMatrix();
	}

	const glm::mat4 &getProjectionMatrix() const noexcept { return this->proj; }

  protected:
	void updateProjectionMatrix() noexcept {
		this->proj = glm::perspective(glm::radians(this->getFOV() * 0.5f), this->aspect, this->near, this->far);
	}

  private:
	float fov = 80.0f;
	float aspect = 16.0f / 9.0f;
	float near = 0.15f;
	float far = 1000.0f;

	float speed = 100;
	float activated = 1.0f;
	float xspeed = 0.5f;
	float yspeed = 0.5f;

	bool enable_Navigation = true;

	int x, y, xprev, yprev;

	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 pos = {0.0f, 1.0f, 0.0f};
	glm::vec3 look = {0.0f, 0.0f, 1.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};
};
