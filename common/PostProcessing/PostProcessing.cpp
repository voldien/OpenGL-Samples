#include "PostProcessing/PostProcessing.h"

using namespace glsample;

PostProcessing::PostProcessing() : computeShaderSupported(glewIsExtensionSupported("GL_ARB_compute_shader")) {}

void PostProcessing::draw(const std::initializer_list<std::tuple<GBuffer, unsigned int>> &render_targets) {

	/*	*/
	for (const auto *it = render_targets.begin(); it != render_targets.end(); it++) {
		GBuffer target = std::get<0>(*it);
		unsigned int texture = std::get<1>(*it);

		if (glBindTextureUnit) {
			glBindTextureUnit(static_cast<unsigned int>(target), texture);
		} else {
			glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(target));
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

float PostProcessing::getIntensity() const noexcept { return this->intensity; }
void PostProcessing::setItensity(const float intensity) { this->intensity = intensity; }

void PostProcessing::addRequireBuffer(const GBuffer required_data_buffer) noexcept {
	this->required_buffer.push_back(required_data_buffer);
}
void PostProcessing::removeRequireBuffer(const GBuffer required_data_buffer) noexcept {}

int PostProcessing::createVAO() {
	unsigned int vao = 0;
	glGenVertexArrays(1, &vao);

	return vao;
}