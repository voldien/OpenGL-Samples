#include "Node.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

using namespace glsample;

void Node::setPosition(const glm::vec3 &globalPosition) noexcept {

	const int num_childrens = this->getNumChildren();
	for (int child_index = 0; child_index < num_childrens; child_index++) {

		Node *child_node = dynamic_cast<Node *>(this->getChild(child_index));
		assert(child_node);
		assert(child_node != this);

		const glm::vec3 position_offset = globalPosition - this->getPosition();
		child_node->setPosition(child_node->getPosition() + position_offset);
	}

	TransformGLM::setPosition(globalPosition);
}
glm::vec3 Node::getPosition() noexcept { return TransformGLM::getPosition(); }
const glm::vec3 &Node::getPosition() const noexcept { return TransformGLM::getPosition(); }

void Node::setScale(const glm::vec3 &globalScale) noexcept {

	const int num_childrens = this->getNumChildren();

	const glm::vec3 scale_ratio = globalScale / this->getScale();

	for (int child_index = 0; child_index < num_childrens; child_index++) {

		Node *child_node = dynamic_cast<Node *>(this->getChild(child_index));
		assert(child_node);
		assert(child_node != this);

		child_node->setScale(child_node->getScale() * scale_ratio);
	}

	TransformGLM::setScale(globalScale);
}
glm::vec3 Node::getScale() const noexcept { return TransformGLM::getScale(); }

const glm::quat &Node::getRotation() const noexcept { return TransformGLM::getRotation(); }
glm::vec3 Node::getRotationEular() const noexcept { return TransformGLM::getRotationEular(); }
void Node::setRotation(const glm::quat &quat) noexcept {

	const int num_childrens = this->getNumChildren();

	const glm::quat rotate_offset = this->getRotation() * glm::inverse(quat);

	for (int child_index = 0; child_index < num_childrens; child_index++) {

		Node *child_node = dynamic_cast<Node *>(this->getChild(child_index));
		assert(child_node);
		assert(child_node != this);

		child_node->setRotation(child_node->getRotation() * rotate_offset);
	}

	TransformGLM::setRotation(quat);
}
void Node::setRotationEular(const glm::vec3 &globalEularRotation) noexcept {
	Node::setRotation(glm::quat(globalEularRotation));
}

void Node::setGlobalPositionDirect(const glm::vec3 &globalPosition) noexcept {
	TransformGLM::setPosition(globalPosition);
}
void Node::setGlobalRotationDirect(const glm::quat &globalRotation) noexcept {
	TransformGLM::setRotation(globalRotation);
}
void Node::setGlobalScaleDirect(const glm::vec3 &globalScale) noexcept { TransformGLM::setScale(globalScale); }

Node *Node::parent() const noexcept { return dynamic_cast<Node *>(this->getParent()); }

glm::mat4 Node::getGlobalMatrix() const noexcept {
	glm::mat4 globalModel(1);
	globalModel = glm::translate(globalModel, this->getPosition());
	globalModel = globalModel * glm::toMat4(this->getRotation());
	globalModel = glm::scale(globalModel, this->getScale());
	return globalModel;
}
glm::mat4 Node::getLocalMatrix() const noexcept {

	glm::mat4 localModel(1);
	/*	*/
	localModel = glm::translate(localModel, this->getLocalPosition());
	localModel = localModel * glm::toMat4(this->getLocalRotation());
	localModel = glm::scale(localModel, this->getLocalScale());
	return localModel;
}

glm::mat4 Node::getViewMatrix() const noexcept {

	glm::mat4 globalView(1);
	/*	*/
	globalView = globalView * glm::toMat4(glm::inverse(this->getRotation()));
	globalView = glm::translate(globalView, -this->getPosition());

	globalView = glm::scale(globalView, this->getScale());

	return globalView;
}

glm::mat4 Node::getLocalViewMatrix() const noexcept {

	glm::mat4 localView(1);
	/*	*/
	localView = glm::translate(localView, -this->getLocalPosition());
	localView = localView * glm::toMat4(glm::inverse(this->getLocalRotation()));
	localView = glm::scale(localView, this->getLocalScale());

	return localView;
}

glm::mat4 Node::getRotationMatrix() const noexcept { return glm::toMat4(this->getRotation()); }

glm::mat4 Node::getLocalRotationMatrix() const noexcept { return glm::toMat4(this->getLocalRotation()); }

glm::mat4 Node::getViewTranslationMatrix() const noexcept { return glm::translate(glm::mat4(1), -this->getPosition()); }

void Node::setLocalPosition(const glm::vec3 &localPosition) noexcept {
	Node *parent = this->parent();

	if (parent) {
		const glm::vec3 global_scale = parent->getPosition();
		this->setPosition(global_scale + localPosition);
	} else {
		this->setPosition(localPosition);
	}
}
void Node::setLocalScale(const glm::vec3 &localScale) noexcept {
	Node *parent = this->parent();
	if (parent) {
		const glm::vec3 global_scale = parent->getScale();
		this->setScale(global_scale * localScale);
	} else {
		this->setScale(localScale);
	}
}
void Node::setLocalRotation(const glm::quat &localRotation) noexcept {
	Node *parent = this->parent();

	if (parent) {
		const glm::quat global_rotation = parent->getRotation();
		this->setRotation(global_rotation * localRotation);
	} else {
		this->setRotation(localRotation);
	}
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
		return this->getScale() * parent->getScale();
	}
	return this->getScale();
}

glm::quat Node::getLocalRotation() const noexcept {
	Node *parent = this->parent();
	if (parent) {
		return parent->getRotation() * glm::inverse(getRotation());
	}

	return this->getRotation();
}