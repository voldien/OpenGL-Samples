#include "Node.h"

using namespace glsample;

void Node::setPosition(const glm::vec3 &position) noexcept {}
glm::vec3 Node::getPosition() noexcept { return {}; }
const glm::vec3 &Node::getPosition() const noexcept { return TransformGLM::getPosition(); }

void Node::setScale(const glm::vec3 &scale) noexcept {}
glm::vec3 Node::getScale() const noexcept { return {}; }

const glm::quat &Node::getRotation() const noexcept { return TransformGLM::getRotation(); }
void Node::setRotation(const glm::quat &quat) noexcept {}

glm::mat4 Node::getViewMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }
glm::mat4 Node::getRotationMatrix() const noexcept {
	glm::quat rotation = glm::quatLookAt(glm::normalize(this->forward()), glm::normalize(this->up()));
	return glm::toMat4(rotation);
}
glm::mat4 Node::getViewTranslationMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }

glm::vec3 Node::getLocalPosition() const noexcept { return {}; }
glm::vec3 Node::getLocalScale() const noexcept { return {}; }
glm::quat Node::getLocalRotation() const noexcept { return {}; }