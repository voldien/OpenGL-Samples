#include "PostProcessing/DepthOfFieldPostProcessing.h"
#include "GLSampleSession.h"
#include "SampleHelper.h"
#include <IOUtil.h>

using namespace glsample;

DepthOfFieldProcessing::DepthOfFieldProcessing() {
	this->setName("Depth Of Field");
	this->addRequireBuffer(GBuffer::Albedo);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
	this->addRequireBuffer(GBuffer::Depth);
}

DepthOfFieldProcessing::~DepthOfFieldProcessing() {
	if (glIsProgram(this->guassian_blur_compute_program)) {
		glDeleteProgram(this->guassian_blur_compute_program);
	}
	if (glIsProgram(this->indirect_compute_program)) {
		glDeleteProgram(this->indirect_compute_program);
	}
}

void DepthOfFieldProcessing::initialize(fragcore::IFileSystem *filesystem) {}

void DepthOfFieldProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {}
