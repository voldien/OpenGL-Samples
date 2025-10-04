#include "Material.h"
#include "RenderDesc.h"
#include "Scene.h"
#include "Scene/RenderQueue.h"

using namespace glsample;

Material::Material() { this->textures.resize(32, nullptr); }

void Material::bindBuffer(const unsigned int index, const UBORange &buffer) {}

void Material::setTexture(const int index, glsample::Texture *texture) { this->textures[index] = texture; }
glsample::Texture *Material::getTexture(const int index) { return this->textures[index]; }

void Material::setSampler(const int index, TextureSampler *sampler) {}
TextureSampler *Material::getSampler(const int index) { return nullptr; }

void Material::setPipeline(ShaderPipeline *pipeline) { /*  Extract information.    */ }

glsample::RenderQueue Material::getRenderQueue() const noexcept { return this->getGraphicSettings().queue; }

glsample::RenderQueue Material::getDefaultQueueDomain(const Material &material) noexcept {

	/*	*/
	const bool useAlphaClipping = (material.getGraphicSettings().clipping > 0.0f);

	/*	*/
	const bool useBlending = ((material.transparent[3] < 1.0f || material.diffuse[3] < 1.0f) ||
							 (material.texture_index[TextureTypeBinding::AlphaMask] >= 0)) &&
							 material.getGraphicSettings().blend_color_func > BlendFunc::One;

	/*	*/
	const bool useWireframe = material.getGraphicSettings().fillMode != FillMode::Fill;

	if (useWireframe) {
		return RenderQueue::Overlay;
	}

	if (useBlending && !useAlphaClipping) {
		return RenderQueue::Transparent;
	}

	if (useAlphaClipping) {
		return RenderQueue::AlphaTest;
	}

	return RenderQueue::Geometry;
}