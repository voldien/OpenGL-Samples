#include "Util/ProcessDataUtil.h"
#include "IOUtil.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

using namespace glsample;

ProcessData::ProcessData(fragcore::IFileSystem *filesystem) : filesystem(filesystem) {}
ProcessData::~ProcessData() {
	if (this->bump2normal_program >= 0) {
		glDeleteProgram(this->bump2normal_program);
	}
	if (this->irradiance_program >= 0) {
		glDeleteProgram(this->irradiance_program);
	}
	if (this->perlin_noise2D_program >= 0) {
		glDeleteProgram(this->perlin_noise2D_program);
	}
}

void ProcessData::computeIrradiance(unsigned int env_source, unsigned int &irradiance_target, const unsigned int width,
									const unsigned int height) {

	glGenTextures(1, &irradiance_target);

	glBindTexture(GL_TEXTURE_2D, irradiance_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

	/*	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	ProcessData::computeIrradiance(env_source, irradiance_target);
}

void ProcessData::computeIrradiance(unsigned int env_source, unsigned int irradiance_target) {

	const char *irradiance_path = "Shaders/compute/irradiance_env.comp.spv";

	if (this->irradiance_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_irradiance_env_binary =
			IOUtil::readFileData<uint32_t>(irradiance_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->irradiance_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_irradiance_env_binary);
		glUseProgram(this->irradiance_program);

		glUniform1i(glGetUniformLocation(this->irradiance_program, "sourceEnvTexture"), 0);
		glUniform1i(glGetUniformLocation(this->irradiance_program, "targetIrradianceTexture"), 1);
	}

	glUseProgram(this->irradiance_program);

	GLint localWorkGroupSize[3];

	glGetProgramiv(this->irradiance_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, irradiance_target);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, env_source);

	/*	Parameters.	*/
	glUniform1f(glGetUniformLocation(this->irradiance_program, "settings.SampleDelta"), 0.025f);
	glUniform1f(glGetUniformLocation(this->irradiance_program, "settings.maxValue"), 64);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(1, irradiance_target, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	glDispatchCompute(WorkGroupX, WorkGroupY, 1);

	glBindTexture(GL_TEXTURE_2D, 0);

	/*	Wait in till image has been written.	*/
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(0);
}

void ProcessData::computePerlinNoise(unsigned int target_texture, const glm::vec2 &size, const glm::vec2 &tile_offset,
									 const int octaves) {
	const char *irradiance_path = "Shaders/compute/perlin_noise_2D_image.comp.spv";

	if (this->perlin_noise2D_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_irradiance_env_binary =
			IOUtil::readFileData<uint32_t>(irradiance_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->perlin_noise2D_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &compute_irradiance_env_binary);
		glUseProgram(this->perlin_noise2D_program);

		glUniform1i(glGetUniformLocation(this->perlin_noise2D_program, "TargetTexture"), 0);
	}

	glUseProgram(this->perlin_noise2D_program);

	GLint localWorkGroupSize[3];

	glGetProgramiv(this->perlin_noise2D_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	GLint width = 0;
	GLint height = 0;

	glBindTexture(GL_TEXTURE_2D, target_texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	/*	Parameters.	*/
	glUniform2fv(glGetUniformLocation(this->perlin_noise2D_program, "settings.offset"), 1, &tile_offset[0]);
	glUniform2fv(glGetUniformLocation(this->perlin_noise2D_program, "settings.size"), 1, &size[0]);
	glUniform2f(glGetUniformLocation(this->perlin_noise2D_program, "settings.ampfreq"), 10.0f, 10.0f);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(0, target_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	glDispatchCompute(WorkGroupX, WorkGroupY, 1);
	/*	Wait in till image has been written.	*/
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_2D, target_texture);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(0);
}

void ProcessData::computePerlinNoise(unsigned int *target, const unsigned int width, const unsigned int height,
									 const glm::vec2 &size, const glm::vec2 &tile_offset, const int octaves) {
	glGenTextures(1, target);

	glBindTexture(GL_TEXTURE_2D, *target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
	/*	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);

	glBindTexture(GL_TEXTURE_2D, 0);

	ProcessData::computePerlinNoise(*target, size, tile_offset, octaves);
}

void ProcessData::computeBump2Normal(unsigned int bump_source, unsigned int &normal_target, const unsigned int width,
									 const unsigned int height) {

	glGenTextures(1, &normal_target);
	glBindTexture(GL_TEXTURE_2D, normal_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	/*	Filtering.	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 5);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	ProcessData::computeBump2Normal(bump_source, normal_target);
}

void ProcessData::computeBump2Normal(unsigned int bump_source, unsigned int normal_target) {
	const char *irradiance_path = "Shaders/compute/bump2normal.comp.spv";

	if (this->bump2normal_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_bump_2_normal_binary =
			IOUtil::readFileData<uint32_t>(irradiance_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->bump2normal_program = ShaderLoader::loadComputeProgram(compilerOptions, &compute_bump_2_normal_binary);

		glUseProgram(this->bump2normal_program);
		glUniform1i(glGetUniformLocation(this->bump2normal_program, "SourceBumpTexture"), 0);
		glUniform1i(glGetUniformLocation(this->bump2normal_program, "TargetNormalTexture"), 1);
	}

	glUseProgram(this->bump2normal_program);

	GLint localWorkGroupSize[3];

	glGetProgramiv(this->bump2normal_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	GLint width = 0;
	GLint height = 0;

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, normal_target);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glBindTexture(GL_TEXTURE_2D, bump_source);

	glUniform1f(glGetUniformLocation(this->bump2normal_program, "settings.strength"), 1);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(1, normal_target, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	glDispatchCompute(WorkGroupX, WorkGroupY, 1);

	/*	Wait in till image has been written.	*/
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_2D, normal_target);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}