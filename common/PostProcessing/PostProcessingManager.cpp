#include "PostProcessing/PostProcessingManager.h"
#include "PostProcessing/PostProcessing.h"

using namespace glsample;

void PostProcessingManager::addPostProcessing(PostProcessing &postProcessing) {
	this->postProcessings.push_back(&postProcessing);
	this->post_enabled.push_back(false);
}

size_t PostProcessingManager::getNrPostProcessing() const noexcept { return this->postProcessings.size(); }
PostProcessing &PostProcessingManager::getPostProcessing(const size_t index) { return *this->postProcessings[index]; }

bool PostProcessingManager::isEnabled(const size_t index) const noexcept {
	return this->post_enabled[index] && this->postProcessings[index]->getIntensity() > 0;
}

void PostProcessingManager::enablePostProcessing(const size_t index, const bool enabled) {
	this->post_enabled[index] = enabled;
}

void PostProcessingManager::render(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) { /*	*/

	/*	Bind Common Data.	*/

	/*	*/
	for (size_t i = 0; i < this->getNrPostProcessing(); i++) {
		if (this->isEnabled(i)) {
			/*	*/
			PostProcessing &postprocessing = getPostProcessing(i);

			postprocessing.bind();

			/*	Render.	*/
			postprocessing.draw(framebuffer, render_targets);
		}
	}
}