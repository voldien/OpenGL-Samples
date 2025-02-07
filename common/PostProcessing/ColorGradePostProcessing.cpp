#include "PostProcessing/ColorGradePostProcessing.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

ColorGradePostProcessing::ColorGradePostProcessing() {
	this->setName("ColorGrade");
	this->addRequireBuffer(GBuffer::Color);
}

ColorGradePostProcessing::~ColorGradePostProcessing() {
	if (this->hue_color_grade_program >= 0) {
		glDeleteProgram(this->hue_color_grade_program);
	}
	if (this->grayscale_color_grade_program >= 0) {
		glDeleteProgram(this->grayscale_color_grade_program);
	}
}

void ColorGradePostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	const char *hue_color_correction_compute_shader_path =
		"Shaders/postprocessingeffects/color/hue_color_correction.comp.spv";
	const char *grayscale_compute_shader_path = "Shaders/postprocessingeffects/color/grayscale.comp.spv";
	const char *sepia_compute_shader_path = "Shaders/postprocessingeffects/color/sepia.comp.spv";
	const char *exposure_compute_shader_path = "Shaders/postprocessingeffects/color/exposure.comp.spv";
	const char *gamma_compute_shader_path = "Shaders/postprocessingeffects/color/gamma.comp.spv";
	const char *color_balance_compute_shader_path = "Shaders/postprocessingeffects/color/color_balance.comp.spv";

	if (this->hue_color_grade_program == -1) {
		/*	*/
		const std::vector<uint32_t> hue_color_binary =
			IOUtil::readFileData<uint32_t>(hue_color_correction_compute_shader_path, filesystem);
		const std::vector<uint32_t> grayscale_compute_binary =
			IOUtil::readFileData<uint32_t>(grayscale_compute_shader_path, filesystem);
		const std::vector<uint32_t> sepia_compute_binary =
			IOUtil::readFileData<uint32_t>(sepia_compute_shader_path, filesystem);
		const std::vector<uint32_t> exposure_compute_binary =
			IOUtil::readFileData<uint32_t>(exposure_compute_shader_path, filesystem);
		const std::vector<uint32_t> gamma_compute_binary =
			IOUtil::readFileData<uint32_t>(gamma_compute_shader_path, filesystem);
		const std::vector<uint32_t> color_balance_binary =
			IOUtil::readFileData<uint32_t>(color_balance_compute_shader_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->hue_color_grade_program = ShaderLoader::loadComputeProgram(compilerOptions, &hue_color_binary);
		this->grayscale_color_grade_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &grayscale_compute_binary);
		this->color_balance_program = ShaderLoader::loadComputeProgram(compilerOptions, &sepia_compute_binary);
		this->gamma_program = ShaderLoader::loadComputeProgram(compilerOptions, &exposure_compute_binary);
		this->exposure_program = ShaderLoader::loadComputeProgram(compilerOptions, &gamma_compute_binary);
		this->sepia_program = ShaderLoader::loadComputeProgram(compilerOptions, &sepia_compute_binary);
	}

	glUseProgram(this->hue_color_grade_program);

	glUniform1i(glGetUniformLocation(this->hue_color_grade_program, "ColorTexture"), 0);
	 glGetProgramiv(this->hue_color_grade_program, GL_COMPUTE_WORK_GROUP_SIZE,
	 			   &this->localWorkGroupSize[0]);
	glUseProgram(0);

	glUseProgram(this->grayscale_color_grade_program);
	glUniform1i(glGetUniformLocation(this->grayscale_color_grade_program, "ColorTexture"), 0);
	glUseProgram(0);

	glUseProgram(this->color_balance_program);
	glUniform1i(glGetUniformLocation(this->color_balance_program, "ColorTexture"), 0);
	glUseProgram(0);

	glUseProgram(this->gamma_program);
	glUniform1i(glGetUniformLocation(this->gamma_program, "ColorTexture"), 0);
	glUseProgram(0);

	glUseProgram(this->exposure_program);
	glUniform1i(glGetUniformLocation(this->exposure_program, "ColorTexture"), 0);
	glUseProgram(0);

	glUseProgram(this->sepia_program);
	glUniform1i(glGetUniformLocation(this->sepia_program, "ColorTexture"), 0);
	glUseProgram(0);
}

void ColorGradePostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);
}

void ColorGradePostProcessing::renderUI() { /*	*/ }