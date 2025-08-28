#include "Camera.h"

using namespace glsample;

Camera::Camera() noexcept { this->updateProjectionMatrix(); }

void Camera::calcFrustumPlanes(const Vector3 &position, const Vector3 &look_forward, const Vector3 &up,
							   const Vector3 &right) {

	/*	*/
	const float halfVSide = this->getFar() * ::tanf(Math::degToRad(this->getFOVDegree()) * 0.5f);
	const float halfHSide = halfVSide * this->getAspect();

	/*	*/
	const Vector3 farDistance = this->getFar() * look_forward;

	/*	*/ // TODO: impl
	switch (getProjectionMode()) {
	case CameraProjectionMode::Orthographic:
		break;
	case CameraProjectionMode::Perspective:

		/*	*/
		this->planes[NEAR_PLANE] = {position + this->getNear() * look_forward, look_forward};
		this->planes[FAR_PLANE] = {position + farDistance, -look_forward};

		/*	*/
		this->planes[RIGHT_PLANE] = {position, (farDistance - right * halfHSide).cross(up)};
		this->planes[LEFT_PLANE] = {position, up.cross(farDistance + right * halfHSide)};

		/*	*/
		this->planes[TOP_PLANE] = {position, right.cross(farDistance - up * halfVSide)};
		this->planes[BOTTOM_PLANE] = {position, (farDistance + up * halfVSide).cross(right)};
		break;
	default:
		break;
	}
}

void Camera::setAspect(const float aspect) noexcept {
	this->aspect = aspect;
	this->updateProjectionMatrix();
}
float Camera::getAspect() const noexcept { return this->aspect; }

void Camera::setNear(const float near) noexcept {
	this->near = near;
	this->updateProjectionMatrix();
}
float Camera::getNear() const noexcept { return this->near; }

void Camera::setFar(const float far) noexcept {
	this->far = far;
	this->updateProjectionMatrix();
}
float Camera::getFar() const noexcept { return this->far; }

float Camera::getFOVDegree() const noexcept { return this->fov_degree; }
void Camera::setFOVDegree(const float FOV_degree) noexcept {
	this->fov_degree = FOV_degree;
	this->updateProjectionMatrix();
}

void Camera::setOrth(const float left, const float right, const float bottom, const float top, const float near,
					 const float far) noexcept {
	this->left = left;
	this->right = right;
	this->bottom = bottom;
	this->top = top;
	this->near = near;
	this->far = far;
}

const glm::mat4 &Camera::getProjectionMatrix() const noexcept { return this->proj; }

void Camera::setMode(const CameraProjectionMode newMode) {
	this->mode = newMode;
	this->updateProjectionMatrix();
}
Camera::CameraProjectionMode Camera::getProjectionMode() const noexcept { return this->mode; }
void Camera::updateProjectionMatrix() noexcept {

	switch (getProjectionMode()) {
	case CameraProjectionMode::Orthographic:
		this->proj = glm::ortho(this->left, this->right, this->bottom, this->top, this->near, this->far);
		break;
	case CameraProjectionMode::Perspective:
	default:
		this->proj = glm::perspective(glm::radians(this->getFOVDegree() * 0.5f), this->aspect, this->near, this->far);
		break;
	}
}
