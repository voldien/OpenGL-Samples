#include "PostProcessing/BloomPostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

BloomPostProcessing::BloomPostProcessing() { this->setName("Bloom"); }

BloomPostProcessing::~BloomPostProcessing() {
	if (this->bloom_blur_compute_program >= 0) {
		glDeleteProgram(this->bloom_blur_compute_program);
	}
}

void BloomPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *sobel_compute_path = "Shaders/postprocessingeffects/gaussian_blur.comp.spv";

	if (this->bloom_blur_compute_program == -1) {
		/*	*/
		const std::vector<uint32_t> guassian_blur_compute_binary =
			IOUtil::readFileData<uint32_t>(sobel_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->bloom_blur_compute_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_compute_binary);
	}

	glUseProgram(this->bloom_blur_compute_program);

	glGetProgramiv(this->bloom_blur_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	glUniform1i(glGetUniformLocation(this->bloom_blur_compute_program, "ColorTexture"), 0);

	glUseProgram(0);

	// glGenVertexArrays(1, &this->vao);
}

void BloomPostProcessing::draw(const std::initializer_list<std::tuple<GBuffer, unsigned int>> &render_targets) {
	unsigned int texture = std::get<1>(*render_targets.begin());

	/*	*/ // TODO: relocate
	for (auto it = render_targets.begin(); it != render_targets.end(); it++) {
		GBuffer target = std::get<0>(*it);
		unsigned int texture = std::get<1>(*it);
		if (glBindTextureUnit) {
			glBindTextureUnit(static_cast<unsigned int>(target), texture);
		} else {
			glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(target));
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	}

	this->convert(texture);
}

void BloomPostProcessing::convert(unsigned int texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glUseProgram(this->bloom_blur_compute_program);

	/*	*/
	glUniform1f(glGetUniformLocation(this->bloom_blur_compute_program, "settings.variance"), 1);
	glUniform1f(glGetUniformLocation(this->bloom_blur_compute_program, "settings.mean"), 0);
	glUniform1f(glGetUniformLocation(this->bloom_blur_compute_program, "settings.radius"), 2);
	glUniform1i(glGetUniformLocation(this->bloom_blur_compute_program, "settings.samples"), 7);

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
