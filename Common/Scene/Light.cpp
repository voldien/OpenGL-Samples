#include "Light.h"

using namespace glsample;

glm::vec3 Light::getDirectionalLight() const noexcept { return this->forward(); }

float Light::getShadowStrength() const noexcept { return this->shadow; }
void Light::setShadowStrength(float strength) { this->shadow = strength; }

void Light::setShadowDistance(float distance) { this->shadowDistance = distance; }
float Light::getShadowDistance() const noexcept { return this->shadowDistance; }

const glm::mat4 &Light::getProjectionMatrix() const noexcept { return this->shadowData.lightSpaceMatrix; }

Light::LightType Light::getLightType() const noexcept { return this->lightType; }

glsample::FrameBuffer *Light::getFrameBuffer() const noexcept { return this->shadowFrameBuffer; }

void Light::setSize(const glm::ivec3 &size) {}
glm::ivec3 Light::getSize() const noexcept {
	return this->shadowFrameBuffer ? shadowFrameBuffer->attachmentSize[shadowFrameBuffer->depthIndex] : glm::ivec3(0);
}

glm::vec4 Light::getColor() const noexcept { return this->color; }
void Light::setColor(const glm::vec4 &newColor) { this->color = newColor; }
