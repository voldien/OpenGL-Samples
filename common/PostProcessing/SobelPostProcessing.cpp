#include "SobelPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

SobelProcessing::SobelProcessing() {
	this->setName("Sobel Edge Detection");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
}

SobelProcessing::~SobelProcessing() {
	if (this->sobel_program >= 0) {
		glDeleteProgram(this->sobel_program);
	}
}

void SobelProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *sobel_compute_path = "Shaders/postprocessingeffects/sobeledgedetection.comp.spv";

	if (this->sobel_program == -1) {
		/*	*/
		const std::vector<uint32_t> sobel_edeg_detection_compute_post_processing_binary =
			IOUtil::readFileData<uint32_t>(sobel_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->sobel_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &sobel_edeg_detection_compute_post_processing_binary);
	}

	glUseProgram(this->sobel_program);
	glGetProgramiv(this->sobel_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);

	glUniform1i(glGetUniformLocation(this->sobel_program, "ColorTexture"), 0);
	glUniform1i(glGetUniformLocation(this->sobel_program, "TargetTexture"), 1);

	glUseProgram(0);
}

void SobelProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->render(framebuffer, this->getMappedBuffer(GBuffer::Color),
				 this->getMappedBuffer(GBuffer::IntermediateTarget));
}

void SobelProcessing::render(glsample::FrameBuffer *framebuffer, unsigned int source_texture,
							 unsigned int target_texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, source_texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glUseProgram(this->sobel_program);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(0, source_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	glBindImageTexture(1, target_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	if (WorkGroupX > 0 && WorkGroupY > 0) {

		glDispatchCompute(WorkGroupX, WorkGroupY, 1);
	}
	glUseProgram(0);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	/*	Swap buffers.	(ping pong)	*/
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
	std::swap(framebuffer->attachments[0], framebuffer->attachments[1]);
}
