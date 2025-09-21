#include "Light.h"

using namespace glsample;

glm::vec3 Light::getDirectionalLight() const noexcept { return this->forward(); }

float Light::getShadowStrength() const noexcept { return this->shadow; }
void Light::setShadowStrength(float strength) { this->shadow = strength; }

void Light::setShadowDistance(float distance) { this->shadowDistance = distance; }
float Light::getShadowDistance() const noexcept { return this->shadowDistance; }

void Light::setShadowFade(float fade) { this->pcf_radius = fade; }
float Light::getShadowFade() const noexcept { return this->pcf_radius; }

const glm::mat4 &Light::getProjectionMatrix() const noexcept { return this->shadowData.lightSpaceMatrix; }

Light::LightType Light::getLightType() const noexcept { return this->lightType; }

glsample::FrameBuffer *Light::getFrameBuffer() const noexcept { return this->shadowFrameBuffer; }

void Light::setSize(const glm::ivec3 &size) {}
glm::ivec3 Light::getSize() const noexcept {
	return this->shadowFrameBuffer ? shadowFrameBuffer->attachmentSize[shadowFrameBuffer->depthIndex] : glm::ivec3(0);
}

glm::vec4 Light::getColor() const noexcept { return this->color; }
void Light::setColor(const glm::vec4 &newColor) { this->color = newColor; }

/////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////

DirectionalLight::DirectionalLight() {
	this->setName("Directional Light");
	this->lightType = LightType::Directional;
	this->setShadowDistance(50.0f);
	this->rotateTowards(glm::vec3(1));
}

void DirectionalLight::setSize(const glm::ivec3 &size) {

	if (size[0] > 0 && size[1] > 0) {

		GraphicFormat internal_depth_format = GraphicFormat::Depth_32Bit;
		fragcore::TextureDesc desc;
		desc.target = fragcore::TextureDesc::TextureTarget::Texture2D;
		desc.width = size[0];
		desc.height = size[1];
		desc.depth = 1;
		desc.graphicFormat = internal_depth_format;
		desc.nrSamples = 0;

		if (!this->getFrameBuffer()) {
			shadowFrameBuffer = new FrameBuffer();
			CommonUtil::createFrameBuffer(shadowFrameBuffer, 0);
		}

		if (this->getFrameBuffer()) {
			CommonUtil::updateFrameBuffer(getFrameBuffer(), {}, &desc);
		}
	}
}

void DirectionalLight::setShadowDistance(float distance) {
	Light::setShadowDistance(distance);

	const float near_plane = -(getShadowDistance());
	const float far_plane = (getShadowDistance());
	const glm::mat4 lightProjection = glm::ortho(-getShadowDistance(), getShadowDistance(), -getShadowDistance(),
												 getShadowDistance(), near_plane, far_plane);

	const glm::vec3 light_direction = this->getDirectionalLight();

	const glm::mat4 lightView =
		glm::lookAt(this->getPosition(), this->getPosition() + this->getDirectionalLight(), this->up());
	const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

	shadowData.lightSpaceMatrix = lightSpaceMatrix;

	this->calcFrustumPlanes(this->getPosition(), this->getDirectionalLight(), this->up(), this->right());
}

void DirectionalLight::calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
										 const glm::vec3 &right) {

	const float distance = this->getShadowDistance();

	this->planes[NEAR_PLANE] = {GLM2E(position - distance * look_forward), GLM2E(look_forward)};
	this->planes[FAR_PLANE] = {GLM2E(position + distance * look_forward), GLM2E(-look_forward)};

	this->planes[RIGHT_PLANE] = {GLM2E(position - distance * right), GLM2E(right)};
	this->planes[LEFT_PLANE] = {GLM2E(position + distance * right), GLM2E(-right)};

	this->planes[TOP_PLANE] = {GLM2E(position - distance * up), GLM2E(up)};
	this->planes[BOTTOM_PLANE] = {GLM2E(position + distance * up), GLM2E(-up)};
}