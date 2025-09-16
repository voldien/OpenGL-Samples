#include "PostProcessing/PostProcessingManager.h"
#include "PostProcessing/PostProcessing.h"
#include <GL/glew.h>
#include <cstddef>

using namespace glsample;

PostProcessingManager::PostProcessingManager(GLSampleBase &base) : base(base) { /*	*/ }

void PostProcessingManager::addPostProcessing(const std::shared_ptr<PostProcessing> &postProcessing) {
	/*	*/
	postProcessing->setManager(*this);
	this->postProcessings.push_back(postProcessing);
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
	const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) { /*	*/

	/*	Bind Common Data.	*/

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
	/*	*/
	for (size_t i = 0; i < this->getNrPostProcessing(); i++) {
		/*	*/
		PostProcessing &postprocessing = getPostProcessing(i);
		if (this->isEnabled(i) && postprocessing.isActive()) {

			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, postprocessing.getName().length(),
							 postprocessing.getName().c_str());

			postprocessing.bind();

			/*	Render.	*/
			postprocessing.draw(framebuffer, render_targets);

			glPopDebugGroup();
		}
	}
}

void PostProcessingManager::update(const float deltaTime) {
	for (size_t i = 0; i < postProcessings.size(); i++) {
		postProcessings[i]->update(deltaTime);
	}
}

void PostProcessingManager::swapPostProcessing(int a, int b) {
	a = fragcore::Math::clamp<int>(a, 0, this->postProcessings.size() - 1);
	b = fragcore::Math::clamp<int>(b, 0, this->postProcessings.size() - 1);

	std::swap(this->postProcessings[a], this->postProcessings[b]);
	std::swap(this->post_enabled[a], this->post_enabled[b]);
}