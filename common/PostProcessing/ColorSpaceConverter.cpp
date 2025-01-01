#include "ColorSpaceConverter.h"
#include "Common.h"
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
}

void ColorSpaceConverter::initialize(fragcore::IFileSystem *filesystem) {

	const char *AES_path = "Shaders/postprocessingeffects/colorspace/aces.comp.spv";
	const char *gamma_path = "Shaders/postprocessingeffects/colorspace/gamma.comp.spv";

	const char *heatwave_path = "Shaders/postprocessingeffects/colorspace/heatmap.comp.spv";
	const char *kronos_pbr_path = "Shaders/postprocessingeffects/colorspace/KhronosPBRNeutral.comp.spv";

	if (this->aes_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_AES_binary = IOUtil::readFileData<uint32_t>(AES_path, filesystem);
		const std::vector<uint32_t> compute_Gamma_binary = IOUtil::readFileData<uint32_t>(gamma_path, filesystem);
		const std::vector<uint32_t> compute_HeatWave_binary = IOUtil::readFileData<uint32_t>(heatwave_path, filesystem);
		const std::vector<uint32_t> compute_Kronas_PBR_binary =
			IOUtil::readFileData<uint32_t>(kronos_pbr_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 410;

		/*  */
		this->aes_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_AES_binary);
		this->gamma_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_Gamma_binary);
		this->falsecolor_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_HeatWave_binary);
		this->kronos_neutral_pbr_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &compute_Kronas_PBR_binary);
	}

	glUseProgram(this->aes_program);

	glUniform1i(glGetUniformLocation(this->aes_program, "texture0"), 1);

	GLint localWorkGroupSize[3];

	glGetProgramiv(this->aes_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	glUseProgram(0);

	{
		glUseProgram(this->gamma_program);

		glUniform1i(glGetUniformLocation(this->gamma_program, "texture0"), 1);

		/*	*/
		glGetProgramiv(this->gamma_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		/*	Parameters.	*/
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.gamma"), getGammeSettings().gamma);
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.exposure"), getGammeSettings().exposure);

		glUseProgram(0);
	}

	{
		glUseProgram(this->falsecolor_program);

		glUniform1i(glGetUniformLocation(this->falsecolor_program, "texture0"), 1);

		glGetProgramiv(this->falsecolor_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		glUseProgram(0);
	}

	{
		glUseProgram(this->kronos_neutral_pbr_program);

		glUniform1i(glGetUniformLocation(this->kronos_neutral_pbr_program, "texture0"), 1);

		glGetProgramiv(this->kronos_neutral_pbr_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		glUseProgram(0);
	}
}

void ColorSpaceConverter::convert(unsigned int texture) {

	GLint width = 0;
	GLint height = 0;

	if (this->getColorSpace() == ColorSpace::Raw) {
		return;
	}

	if (this->getColorSpace() != ColorSpace::Raw) {
		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	switch (this->getColorSpace()) {
	case ColorSpace::ACES: {

		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		GLint localWorkGroupSize[3];

		glUseProgram(this->aes_program);

		glGetProgramiv(this->aes_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
		const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::FalseColor: {

		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		GLint localWorkGroupSize[3];

		glUseProgram(this->falsecolor_program);

		glGetProgramiv(this->falsecolor_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
		const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::KhronosPBRNeutral: {
		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		GLint localWorkGroupSize[3];

		glUseProgram(this->kronos_neutral_pbr_program);

		glGetProgramiv(this->kronos_neutral_pbr_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

		/*	The image where the graphic version will be stored as.	*/
		glBindImageTexture(1, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
		const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

		if (WorkGroupX > 0 && WorkGroupY > 0) {

			glDispatchCompute(WorkGroupX, WorkGroupY, 1);
		}
		glUseProgram(0);
	} break;
	case ColorSpace::SRGB:
		break;
	case ColorSpace::Raw:
	default:
		return;
	}

	if (this->getColorSpace() != ColorSpace::Raw) {
		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	/*	*/
	if (this->getColorSpace() != ColorSpace::Raw) {

		/*	Wait in till image has been written.	*/
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindTexture(GL_TEXTURE_2D, texture);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		GLint localWorkGroupSize[3];

		glUseProgram(this->gamma_program);
		/*	Parameters.	*/
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.gamma"), getGammeSettings().gamma);
		glUniform1f(glGetUniformLocation(this->gamma_program, "settings.exposure"), getGammeSettings().exposure);

		glGetProgramiv(this->gamma_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

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