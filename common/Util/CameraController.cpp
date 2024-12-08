#include "CameraController.h"
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

using namespace glsample;

void CameraController::update(const float deltaTime) noexcept {

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
								 current_speed, 0.5f * activated, this->fov_degree, xDiff, yDiff, w, a, s, d, 0, 0, 0);

		/*	*/
		this->updateFrustum();
	}
}

void CameraController::enableNavigation(const bool enable) noexcept { this->enabled_Navigation = enable; }
void CameraController::enableLook(const bool enable) noexcept { this->enabled_Look = enable; }

const glm::mat4 &CameraController::getViewMatrix() const noexcept { return this->view; }
const glm::mat4 CameraController::getRotationMatrix() const noexcept {
	glm::quat rotation = glm::quatLookAt(glm::normalize(this->getLookDirection()), glm::normalize(this->getUp()));
	return glm::toMat4(rotation);
}
const glm::mat4 CameraController::getViewTranslationMatrix() const noexcept {
	return glm::translate(glm::mat4(1), -this->getPosition());
}

const glm::vec3 &CameraController::getLookDirection() const noexcept { return this->look; }
const glm::vec3 CameraController::getPosition() const noexcept { return this->pos; }
void CameraController::setPosition(const glm::vec3 &position) noexcept {
	this->pos = position;
	this->update();
}

const glm::vec3 &CameraController::getUp() const noexcept { return this->up; }

void CameraController::lookAt(const glm::vec3 &position) noexcept {
	this->look = glm::normalize(position - this->getPosition());
	this->update();
}

bool CameraController::hasMoved() const noexcept { return true; }

void CameraController::update() noexcept {
	flythrough_camera_update(&this->pos[0], &this->look[0], &this->up[0], &this->view[0][0], 0, 0, 0.5f * activated,
							 this->fov_degree, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void CameraController::updateFrustum() {

	/*	*/
	const Vector3 position = Vector3(this->pos[0], this->pos[1], this->pos[2]);
	const Vector3 look = Vector3(this->look[0], this->look[1], this->look[2]).normalized();
	const Vector3 up = Vector3(this->up[0], this->up[1], this->up[2]).normalized();
	const Vector3 right = look.cross(up).normalized();

	this->calcFrustumPlanes(position, look, up, right);
}