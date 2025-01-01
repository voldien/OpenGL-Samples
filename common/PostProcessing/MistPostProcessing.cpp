#include "MistPostProcessing.h"
#include "Core/Object.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

MistPostProcessing::MistPostProcessing() {
	this->setName("MistFog");
}

MistPostProcessing::~MistPostProcessing() {
	if (this->mist_program >= 0) {
		glDeleteProgram(this->mist_program);
	}
}

void MistPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *frag_path = "Shaders/postprocessingeffects/mistfog.frag.spv";
	const char *vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->mist_program == -1) {
		/*	*/
		const std::vector<uint32_t> vertex_mistfog_post_processing_binary =
			IOUtil::readFileData<uint32_t>(vertex_path, filesystem);

		const std::vector<uint32_t> fragment_mistfog_post_processing_binary =
			IOUtil::readFileData<uint32_t>(frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->mist_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_mistfog_post_processing_binary,
															  &fragment_mistfog_post_processing_binary);
	}

	glUseProgram(this->mist_program);

	glUniform1i(glGetUniformLocation(this->mist_program, "texture0"), 0);
	glUniform1i(glGetUniformLocation(this->mist_program, "DepthTexture"), 1);
	glUniform1i(glGetUniformLocation(this->mist_program, "IrradianceTexture"), 2);

	glUseProgram(0);

	glGenVertexArrays(1, &this->vao);
}

void MistPostProcessing::render(unsigned int skybox, unsigned int frame_texture, unsigned int depth_texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindVertexArray(this->vao);

	glUseProgram(this->mist_program);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, frame_texture);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, depth_texture);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, skybox);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);
}