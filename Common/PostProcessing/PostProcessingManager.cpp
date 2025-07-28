#include "PostProcessing/PostProcessingManager.h"
#include "DataStructure/MemoryAddress.h"
#include "PostProcessing/PostProcessing.h"
#include <GL/glew.h>

using namespace glsample;

PostProcessingManager::PostProcessingManager() {

	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint *)&this->ubo_pool.buffer.alignment);
	this->ubo_pool.buffer.size = 1024 * 1024 * 2;
	this->ubo_pool.buffer.totalSize =
		fragcore::Math::align<size_t>(this->ubo_pool.buffer.size, (size_t)this->ubo_pool.buffer.alignment);
	this->ubo_pool.addresser = MemoryAddress(this->ubo_pool.buffer.totalSize, 0);

	/*	*/
	glGenBuffers(1, &this->ubo_pool.buffer.buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->ubo_pool.buffer.buffer);
	glBufferData(GL_UNIFORM_BUFFER, this->ubo_pool.buffer.totalSize, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

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

void PostProcessingManager::swapPostProcessing(int a, int b) {
	a = Math::clamp<int>(a, 0, this->postProcessings.size() - 1);
	b = Math::clamp<int>(b, 0, this->postProcessings.size() - 1);

	std::swap(this->postProcessings[a], this->postProcessings[b]);
	std::swap(this->post_enabled[a], this->post_enabled[b]);
}