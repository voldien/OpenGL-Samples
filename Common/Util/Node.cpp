#include "Node.h"

using namespace glsample;

glm::mat4 Node::getViewMatrix() const noexcept {
	 return glm::translate(glm::mat4(1), -this->getPosition()); }
glm::mat4 Node::getRotationMatrix() const noexcept {
	glm::quat rotation = glm::quatLookAt(glm::normalize(this->forward()), glm::normalize(this->up()));
	return glm::toMat4(rotation);
}
glm::mat4 Node::getViewTranslationMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }
