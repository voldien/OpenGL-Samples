#include "PostProcessing/SSSPostProcessing.h"

using namespace glsample;

SSSPostProcessing::SSSPostProcessing() { this->setName("Space Space Shadowing"); }
SSSPostProcessing::~SSSPostProcessing() {}

void SSSPostProcessing::initialize(fragcore::IFileSystem *filesystem) {}

void SSSPostProcessing::draw(glsample::FrameBuffer* framebuffer, const std::initializer_list<std::tuple<const GBuffer, const unsigned int&>> &render_targets) {}
