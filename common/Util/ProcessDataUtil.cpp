#include "Util/ProcessDataUtil.h"
#include "IOUtil.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
using namespace glsample;

ProcessData::ProcessData(fragcore::IFileSystem *filesystem) : filesystem(filesystem) {}
ProcessData::~ProcessData() {
	if (this->bump2normal_program >= 0) {
		glDeleteProgram(this->bump2normal_program);
	}
	if (this->irradiance_program >= 0) {
		glDeleteProgram(this->irradiance_program);
	}
}

void ProcessData::computeIrradiance(unsigned int env_source, unsigned int &irradiance_target, const unsigned int width,
									const unsigned int height) {
	glGenTextures(1, &irradiance_target);
	glBindTexture(GL_TEXTURE_2D, irradiance_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	ProcessData::computeIrradiance(env_source, irradiance_target);
}

void ProcessData::computeIrradiance(unsigned int env_source, unsigned int irradiance_target) {

	const char *irradiance_path = "Shaders/compute/irradiance_env.comp.spv";

	if (this->irradiance_program == -1) {
		/*	*/
		const std::vector<uint32_t> compute_marching_cube_generator_binary =
			IOUtil::readFileData<uint32_t>(irradiance_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 450;

		/*  */
		this->irradiance_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &compute_marching_cube_generator_binary);
	}

	glUseProgram(this->irradiance_program);

	glUniform1i(glGetUniformLocation(this->irradiance_program, "sourceEnvTexture"), 0);
	glUniform1i(glGetUniformLocation(this->irradiance_program, "targetIrradianceTexture"), 1);

	GLint localWorkGroupSize[3];

	glGetProgramiv(this->irradiance_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

	GLint width;
	GLint height;

	glBindTexture(GL_TEXTURE_2D, irradiance_target);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, env_source);

	/*	The image where the graphic version will be stored as.	*/
	glBindImageTexture(1, irradiance_target, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(std::ceil(width / (float)localWorkGroupSize[0]), std::ceil(height / (float)localWorkGroupSize[1]),
					  1);

	glBindTexture(GL_TEXTURE_2D, 0);

	/*	Wait in till image has been written.	*/
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glUseProgram(0);
}