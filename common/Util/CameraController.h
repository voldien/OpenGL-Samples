#pragma once
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#define FLYTHROUGH_CAMERA_IMPLEMENTATION 1
#include "flythrough_camera.h"
#include <glm/gtc/matrix_transform.hpp>

class Camera {
  public:
	Camera() { this->updateProjectionMatrix(); }

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

  protected:
	float fov = 80.0f;
	float aspect = 16.0f / 9.0f;
	float near = 0.15f;
	float far = 1000.0f;
	glm::mat4 proj;
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
		float current_speed = this->speed;
		if (shift) {
			current_speed *= 2.5f;
		}

		/*	*/
		if (!alt) {
			flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], deltaTime,
									 current_speed, 0.5f * activated, this->fov, xDiff, yDiff, w, a, s, d, 0, 0, 0);
		}
	}
	void enableNavigation(const bool enable)noexcept { this->enable_Navigation = enable; }

	const glm::mat4 &getViewMatrix() const noexcept { return this->view; }
	const glm::mat4 getRotationMatrix() const noexcept {
		glm::quat rotation = glm::quatLookAt(glm::normalize(this->getLookDirection()), glm::normalize(this->getUp()));
		return glm::toMat4(rotation);
	}

	const glm::vec3 &getLookDirection() const noexcept { return this->look; }
	const glm::vec3 getPosition() const noexcept { return this->pos; }
	void setPosition(const glm::vec3 &position) noexcept { this->pos = position; }

	const glm::vec3 &getUp() const { return this->up; }

	void lookAt(const glm::vec3 &position) noexcept { this->look = glm::normalize(position - this->getPosition()); }

  private:
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
