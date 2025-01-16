#include "BlurPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

BlurPostProcessing::BlurPostProcessing() {
	this->setName("Blur");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
}

BlurPostProcessing::~BlurPostProcessing() {
	if (this->guassian_blur_compute_program >= 0) {
		glDeleteProgram(this->guassian_blur_compute_program);
	}
}

void BlurPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *sobel_compute_path = "Shaders/postprocessingeffects/gaussian_blur.comp.spv";

	if (this->guassian_blur_compute_program == -1) {
		/*	*/
		const std::vector<uint32_t> guassian_blur_compute_binary =
			IOUtil::readFileData<uint32_t>(sobel_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->guassian_blur_compute_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_compute_binary);
	}

	glUseProgram(this->guassian_blur_compute_program);

	glGetProgramiv(this->guassian_blur_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	glUniform1i(glGetUniformLocation(this->guassian_blur_compute_program, "ColorTexture"), 0);
	glUniform1i(glGetUniformLocation(this->guassian_blur_compute_program, "TargetTexture"), 1);

	glUseProgram(0);
}

void BlurPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);
	this->convert(this->getMappedBuffer(GBuffer::Color));
}

void BlurPostProcessing::convert(unsigned int texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glUseProgram(this->guassian_blur_compute_program);

	/*	*/
	glUniform1f(glGetUniformLocation(this->guassian_blur_compute_program, "settings.variance"), 1);
	glUniform1f(glGetUniformLocation(this->guassian_blur_compute_program, "settings.mean"), 0);
	glUniform1f(glGetUniformLocation(this->guassian_blur_compute_program, "settings.radius"), 2);
	glUniform1i(glGetUniformLocation(this->guassian_blur_compute_program, "settings.samples"), 7);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	if (WorkGroupX > 0 && WorkGroupY > 0) {

		glDispatchCompute(WorkGroupX, WorkGroupY, 1);
	}
	glUseProgram(0);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
