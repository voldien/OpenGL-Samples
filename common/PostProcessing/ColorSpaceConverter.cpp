#include "ColorSpaceConverter.h"
#include "Common.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

ColorSpaceConverter::~ColorSpaceConverter() {
	if (this->aes_program > 0) {
		glDeleteProgram(this->aes_program);
	}
	if (this->gamma_program > 0) {
		glDeleteProgram(this->gamma_program);
	}
	if (this->falsecolor_program > 0) {
		glDeleteProgram(this->falsecolor_program);
	}
	if (this->filmic_program > 0) {
		glDeleteProgram(this->filmic_program);
	}
	if (this->kronos_neutral_pbr_program > 0) {
		glDeleteProgram(this->kronos_neutral_pbr_program);
	}
}

void ColorSpaceConverter::initialize(fragcore::IFileSystem *filesystem) {

	const char *AES_path = "Shaders/postprocessingeffects/colorspace/aces.comp.spv";
	const char *gamma_path = "Shaders/postprocessingeffects/colorspace/gamma.comp.spv";

	const char *heatwave_path = "Shaders/postprocessingeffects/colorspace/heatmap.comp.spv";
	const char *kronos_pbr_path = "Shaders/postprocessingeffects/colorspace/KhronosPBRNeutral.comp.spv";
	const char *filmic_path = "Shaders/postprocessingeffects/colorspace/filmic.comp.spv";

	if (this->aes_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_AES_binary = IOUtil::readFileData<uint32_t>(AES_path, filesystem);
		const std::vector<uint32_t> compute_Gamma_binary = IOUtil::readFileData<uint32_t>(gamma_path, filesystem);
		const std::vector<uint32_t> compute_HeatWave_binary = IOUtil::readFileData<uint32_t>(heatwave_path, filesystem);
		const std::vector<uint32_t> compute_Kronas_PBR_binary =
			IOUtil::readFileData<uint32_t>(kronos_pbr_path, filesystem);
		const std::vector<uint32_t> compute_filmic_binary = IOUtil::readFileData<uint32_t>(filmic_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->aes_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_AES_binary);
		this->gamma_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_Gamma_binary);
		this->falsecolor_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_HeatWave_binary);
		this->kronos_neutral_pbr_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &compute_Kronas_PBR_binary);
		this->filmic_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_filmic_binary);
	}

	glUseProgram(this->aes_program);

	glUniform1i(glGetUniformLocation(this->aes_program, "ColorTexture"), 1);

	glGetProgramiv(this->aes_program, GL_COMPUTE_WORK_GROUP_SIZE,
				   &this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::ACES * 3]);

	glUseProgram(0);

	{
		glUseProgram(this->gamma_program);

		glUniform1i(glGetUniformLocation(this->gamma_program, "ColorTexture"), 1);

		/*	*/
		glGetProgramiv(this->gamma_program, GL_COMPUTE_WORK_GROUP_SIZE,
					   &this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::SRGB * 3]);

		/*	Parameters.	*/
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.gamma"), getGammeSettings().gamma);
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.exposure"), getGammeSettings().exposure);

		glUseProgram(0);
	}

	{
		glUseProgram(this->falsecolor_program);

		glUniform1i(glGetUniformLocation(this->falsecolor_program, "ColorTexture"), 1);

		glGetProgramiv(this->falsecolor_program, GL_COMPUTE_WORK_GROUP_SIZE,
					   &this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::FalseColor * 3]);

		glUseProgram(0);
	}

	{
		glUseProgram(this->kronos_neutral_pbr_program);

		glUniform1i(glGetUniformLocation(this->kronos_neutral_pbr_program, "ColorTexture"), 1);

		glGetProgramiv(this->kronos_neutral_pbr_program, GL_COMPUTE_WORK_GROUP_SIZE,
					   &this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::KhronosPBRNeutral * 3]);

		glUseProgram(0);
	}

	{
		glUseProgram(this->filmic_program);

		glUniform1i(glGetUniformLocation(this->filmic_program, "ColorTexture"), 1);

		glGetProgramiv(this->filmic_program, GL_COMPUTE_WORK_GROUP_SIZE,
					   &this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::Filmic * 3]);

		glUseProgram(0);
	}
}

void ColorSpaceConverter::draw(glsample::FrameBuffer* framebuffer, const std::initializer_list<std::tuple<const GBuffer, const unsigned int&>> &render_targets) {}

void ColorSpaceConverter::convert(unsigned int texture) {

	GLint width = 0;
	GLint height = 0;

	/*	*/
	if (this->getColorSpace() == ColorSpace::RawLinear) {
		return;
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	const GLint *localWorkGroupSize =
		&this->compute_programs_local_workgroup_sizes[(unsigned int)this->getColorSpace() * 3];

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	if (this->getColorSpace() != ColorSpace::RawLinear) {
		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	switch (this->getColorSpace()) {
	case ColorSpace::ACES: {

		GLint localWorkGroupSize[3];

		glUseProgram(this->aes_program);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::FalseColor: {

		glUseProgram(this->falsecolor_program);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::KhronosPBRNeutral: {

		glUseProgram(this->kronos_neutral_pbr_program);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::Filmic: {

		glUseProgram(this->filmic_program);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::SRGB:
		break;
	case ColorSpace::RawLinear:
	default:
		return;
	}

	if (this->getColorSpace() != ColorSpace::RawLinear) {
		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	/*	*/
	if (this->getColorSpace() != ColorSpace::RawLinear) {

		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		const int *localWorkGroupSize =
			&this->compute_programs_local_workgroup_sizes[(unsigned int)ColorSpace::SRGB * 3];

		glUseProgram(this->gamma_program);
		/*	Parameters.	*/
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.gamma"), getGammeSettings().gamma);
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.exposure"), getGammeSettings().exposure);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
		const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	}
}

void ColorSpaceConverter::setColorSpace(const glsample::ColorSpace srgb) noexcept { this->colorSpace = srgb; }
glsample::ColorSpace ColorSpaceConverter::getColorSpace() const noexcept { return this->colorSpace; }