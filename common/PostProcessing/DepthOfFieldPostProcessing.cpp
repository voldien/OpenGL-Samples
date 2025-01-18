#include "PostProcessing/DepthOfFieldPostProcessing.h"

using namespace glsample;

DepthOfFieldProcessing::DepthOfFieldProcessing() {
	this->setName("Depth Of Field");
	this->addRequireBuffer(GBuffer::Albedo);
	this->addRequireBuffer(GBuffer::Depth);
}
DepthOfFieldProcessing::~DepthOfFieldProcessing() {}

void DepthOfFieldProcessing::initialize(fragcore::IFileSystem *filesystem) {}

void DepthOfFieldProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {}
