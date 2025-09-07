#include "Node.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace glsample;

void Node::setPosition(const glm::vec3 &position) noexcept {

	const glm::vec3 position_offset = position - this->getPosition();

	for (int x = 0; x < this->getNumChildren(); x++) {

		Node *node = dynamic_cast<Node *>(this->getChild(x));
		node->setPosition(node->getPosition() + position_offset);
	}

	TransformGLM::setPosition(position);
}
glm::vec3 Node::getPosition() noexcept { return TransformGLM::getPosition(); }
const glm::vec3 &Node::getPosition() const noexcept { return TransformGLM::getPosition(); }

void Node::setScale(const glm::vec3 &scale) noexcept {

	const glm::vec3 scale_offset = scale - getScale();

	TransformGLM::setScale(scale);
}
glm::vec3 Node::getScale() const noexcept { return TransformGLM::getScale(); }

const glm::quat &Node::getRotation() const noexcept { return TransformGLM::getRotation(); }
void Node::setRotation(const glm::quat &quat) noexcept { TransformGLM::setRotation(quat); }

Node *Node::parent() const noexcept { return dynamic_cast<Node *>(this->getParent()); }

glm::mat4 Node::getGlobalMatrix() const noexcept {
	glm::mat4 model(1);
	model = glm::translate(model, this->getPosition());
	model = model * glm::toMat4(this->getRotation());
	model = glm::scale(model, this->getScale());
	return model;
}
glm::mat4 Node::getLocalMatrix() const noexcept {
	glm::mat4 model(1);
	model = glm::translate(model, this->getLocalPosition());
	model = model * glm::toMat4(this->getLocalRotation());
	model = glm::scale(model, this->getLocalScale());
	return model;
}

glm::mat4 Node::getViewMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }
glm::mat4 Node::getLocalViewMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getLocalPosition()); }
glm::mat4 Node::getRotationMatrix() const noexcept {
	glm::quat rotation = glm::quatLookAt(glm::normalize(this->forward()), glm::normalize(this->up()));
	return glm::toMat4(rotation);
}
glm::mat4 Node::getViewTranslationMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }

void Node::setLocalPosition(const glm::vec3 &localPosition) noexcept {
	Node *parent = this->parent();

	const glm::vec3 global_position = parent ? parent->getPosition() : glm::vec3(0);

	this->setPosition(global_position + localPosition);
}
void Node::setLocalScale(const glm::vec3 &localScale) noexcept {
	Node *parent = this->parent();

	const glm::vec3 global_scale = parent ? parent->getScale() : glm::vec3(0);
	this->setScale(global_scale + localScale);
}
void Node::setLocalRotation(const glm::quat &localRotation) noexcept {
	Node *parent = this->parent();
	if (parent) {
	}

	const glm::quat global_rotation = parent ? parent->getRotation() : glm::quat();
	this->setRotation(global_rotation * localRotation);
}

glm::vec3 Node::getLocalPosition() const noexcept {
	Node *parent = this->parent();
	if (parent) {
		return this->getPosition() - parent->getPosition();
	}

	return this->getPosition();
}

glm::vec3 Node::getLocalScale() const noexcept {
	Node *parent = this->parent();
	if (parent) {
		return this->getScale() - parent->getScale();
	}
	return this->getScale();
}
glm::quat Node::getLocalRotation() const noexcept {
	Node *parent = this->parent();
	glm::quat parent_rotation = parent ? parent->getRotation() : glm::quat();

	// TODO: fix
	return parent_rotation * glm::inverse(getRotation());
}