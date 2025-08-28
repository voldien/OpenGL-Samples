#include "Transform.h"
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace glsample;

template class fragcore::Transform<glm::vec3, glm::mat3x3, glm::mat4x4, glm::quat>;

/*	Forward Declare */
template <> glm::vec3 TransformGLM::up() const noexcept;
template <> glm::vec3 TransformGLM::right() const noexcept;
template <> glm::vec3 TransformGLM::forward() const noexcept;

template <>
TransformGLM::Transform(const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &scale)
	: position(position), quat(rotation), scale(scale) {}

template <> TransformGLM::Transform(const glm::mat4x4 &transform) {}

template <> TransformGLM::Transform(const glm::mat3x3 &transform) {}

template <> TransformGLM::Transform(const glm::mat3x3 &basis, const glm::vec3 &c) {}

// template <>
// TransformGLM::Transform(const TransformGLM &other) : position(other.position), quat(other.quat), scale(other.scale)
// {}

template <> TransformGLM &TransformGLM::operator=(const TransformGLM &other) {
	this->position = other.position;
	this->quat = other.quat;
	this->scale = other.scale;
	return *this;
}

template <> void TransformGLM::rotate(const glm::vec3 &eular) noexcept {
	glm::quat eular_quaternion = glm::quat(eular);
	this->quat = this->quat * eular_quaternion;
}

template <> void TransformGLM::rotateTowards(const glm::vec3 &direction) noexcept {
	const glm::vec3 up = this->up();
	this->quat = glm::quatLookAt(direction, up);
}

template <> void TransformGLM::setPosition(const glm::vec3 &position) noexcept { this->position = position; }
template <> glm::vec3 TransformGLM::getPosition() noexcept { return this->position; }
template <> const glm::vec3 &TransformGLM::getPosition() const noexcept { return this->position; }

template <> void TransformGLM::setScale(const glm::vec3 &scale) noexcept { this->scale = scale; }
template <> glm::vec3 TransformGLM::getScale() const noexcept { return this->scale; }

template <> void TransformGLM::setRotation(const glm::quat &quat) noexcept { this->quat = quat; }
template <> const glm::quat &TransformGLM::getRotation() const noexcept { return this->quat; }
template <> glm::vec3 TransformGLM::getRotationEular() const noexcept { return glm::eulerAngles(this->quat); }

template <> void TransformGLM::setRotationEular(const glm::vec3 &eular) noexcept { this->quat = glm::quat(eular); }

template <> TransformGLM TransformGLM::inverse() const noexcept {
	Transform transform{};
	// Matrix3x3 inv = this->getBasis().transpose();
	return transform;
}

template <> glm::mat3x3 TransformGLM::getBasis() const noexcept { return glm::mat3_cast(this->quat); }

template <> TransformGLM &TransformGLM::operator*=([[maybe_unused]] const TransformGLM &t) noexcept {
	// Matrix3x3 basis = this->getBasis() * t.getBasis();
	return *this;
}

template <> TransformGLM TransformGLM::operator*([[maybe_unused]] const TransformGLM &t) const noexcept {
	// Matrix3x3 basis = this->getBasis() * t.getBasis();

	return *this;
}

template <> glm::vec3 TransformGLM::operator*(const glm::vec3 &vector) const noexcept {
	return this->getBasis() * vector;
}

template <> glm::quat TransformGLM::operator*(const glm::quat &quat) const noexcept {
	return this->getRotation() * quat;
}

template <> glm::vec3 TransformGLM::up() const noexcept { return glm::normalize(this->quat * glm::vec3(0, 1, 0)); }
template <> glm::vec3 TransformGLM::right() const noexcept { return glm::normalize(this->quat * glm::vec3(1, 0, 0)); }
template <> glm::vec3 TransformGLM::forward() const noexcept { return glm::normalize(this->quat * glm::vec3(0, 0, 1)); }
