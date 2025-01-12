#include "SobelPostProcessing.h"
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

	glUniform1i(glGetUniformLocation(this->sobel_program, "ColorTexture"), 0);

	glUseProgram(0);
}

void SobelProcessing::draw(const std::initializer_list<std::tuple<GBuffer, unsigned int>> &render_targets) {
	unsigned int texture = std::get<1>(*render_targets.begin());
	this->render(texture);
}

void SobelProcessing::render(unsigned int texture) {
	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	GLint localWorkGroupSize[3];

	glUseProgram(this->sobel_program);

	glGetProgramiv(this->sobel_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

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
