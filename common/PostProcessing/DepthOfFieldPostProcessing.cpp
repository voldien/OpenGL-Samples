#include "PostProcessing/DepthOfFieldPostProcessing.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include "magic_enum.hpp"
#include <GL/glew.h>
#include <IOUtil.h>
#include <limits>

using namespace glsample;

DepthOfFieldProcessing::DepthOfFieldProcessing() {

	this->setName("Depth Of Field");
	this->addRequireBuffer(GBuffer::Albedo);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
	this->addRequireBuffer(GBuffer::Depth);
}

DepthOfFieldProcessing::~DepthOfFieldProcessing() {
	if (glIsProgram(this->guassian_blur_variable_compute_program)) {
		glDeleteProgram(this->guassian_blur_variable_compute_program);
	}
	if (glIsProgram(this->guassian_blur_fixed_compute_program)) {
		glDeleteProgram(this->guassian_blur_fixed_compute_program);
	}
	if (glIsProgram(this->indirect_guassian_dispatch_compute_program)) {
		glDeleteProgram(this->indirect_guassian_dispatch_compute_program);
	}
}

void DepthOfFieldProcessing::initialize(fragcore::IFileSystem *filesystem) {
	/*	*/
	const char *guassian_vertical_blur_compute_path = "Shaders/postprocessingeffects/guassian_blur_vertical.comp.spv";
	const char *guassian_horizontal_blur_compute_path =
		"Shaders/postprocessingeffects/guassian_blur_horizontal.comp.spv";

	const char *guassian_indirect_dispatch_compute_path =
		"Shaders/postprocessingeffects/guassian_blur_vertical.comp.spv";

	const char *box_blur_compute_path = "Shaders/postprocessingeffects/box_blur.comp.spv";

	fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
	compilerOptions.target = fragcore::ShaderLanguage::GLSL;
	compilerOptions.glslVersion = 420; /*	Min required glsl spec.	*/

	if (this->guassian_blur_fixed_compute_program <= 0) {
		/*	*/
		const std::vector<uint32_t> guassian_blur_vertical_compute_binary =
			IOUtil::readFileData<uint32_t>(guassian_vertical_blur_compute_path, filesystem);

		const std::vector<uint32_t> guassian_blur_horizontal_compute_binary =
			IOUtil::readFileData<uint32_t>(guassian_horizontal_blur_compute_path, filesystem);

		/*  */
		// this->guassian_blur_vertical_compute_program =
		//	ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_vertical_compute_binary);
		/*  */
		// this->guassian_blur_horizontal_compute_program =
		//	ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_horizontal_compute_binary);
	}

	if (this->guassian_blur_variable_compute_program <= 0) {
	}

	if (this->indirect_guassian_dispatch_compute_program <= 0) {
		/*	*/
		const std::vector<uint32_t> guassian_indirect_dispatch_compute_binary =
			IOUtil::readFileData<uint32_t>(guassian_vertical_blur_compute_path, filesystem);
	}

	if (guassian_blur_variable_compute_program) {
		glUseProgram(this->guassian_blur_variable_compute_program);
		glGetProgramiv(this->guassian_blur_variable_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize[0]);
		glUniform1i(glGetUniformLocation(this->guassian_blur_variable_compute_program, "ColorTexture"), 0);
		glUniform1i(glGetUniformLocation(this->guassian_blur_variable_compute_program, "TargetTexture"), 1);
		glUseProgram(0);
	}

	if (guassian_blur_fixed_compute_program) {
		glUseProgram(this->guassian_blur_fixed_compute_program);
		glGetProgramiv(this->guassian_blur_fixed_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize[1]);
		glUniform1i(glGetUniformLocation(this->guassian_blur_fixed_compute_program, "ColorTexture"), 0);
		glUniform1i(glGetUniformLocation(this->guassian_blur_fixed_compute_program, "TargetTexture"), 1);
		glUseProgram(0);
	}

	if (indirect_guassian_dispatch_compute_program) {
		glUseProgram(this->indirect_guassian_dispatch_compute_program);
		glGetProgramiv(this->indirect_guassian_dispatch_compute_program, GL_COMPUTE_WORK_GROUP_SIZE,
					   localWorkGroupSize[2]);
		glUniform1i(glGetUniformLocation(this->indirect_guassian_dispatch_compute_program, "ColorTexture"), 0);
		glUniform1i(glGetUniformLocation(this->indirect_guassian_dispatch_compute_program, "TargetTexture"), 1);
		glUseProgram(0);
	}

	/*	Create sampler for sampling GBuffer regardless of the texture internal sampler.	*/
	glCreateSamplers(1, &this->texture_sampler);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameterf(this->texture_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAX_LOD, 0);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_LOD, 0);

	this->vao = createVAO();
}

void DepthOfFieldProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {}
