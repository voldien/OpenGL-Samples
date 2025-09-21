#include "Material.h"
#include "Scene/RenderQueue.h"

using namespace glsample;

Material::Material() { this->textures.resize(32, nullptr); }

void Material::bindBuffer(const unsigned int index, const UBORange &buffer) {}

void Material::setTexture(const int index, glsample::Texture *texture) { this->textures[index] = texture; }
glsample::Texture *Material::getTexture(const int index) { return this->textures[index]; }

void Material::setSampler(const int index, TextureSampler *sampler) {}
TextureSampler *Material::getSampler(const int index) { return nullptr; }

void Material::setPipeline(ShaderPipeline *pipeline) { /*  Extract information.    */ }

glsample::RenderQueue Material::getRenderQueue() const noexcept { return RenderQueue::Geometry; }