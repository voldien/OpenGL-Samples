#include "MistPostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

MistPostProcessing::MistPostProcessing() { this->setName("MistFog"); }

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

	glUniform1i(glGetUniformLocation(this->mist_program, "texture0"), (int)GBuffer::Albedo);
	glUniform1i(glGetUniformLocation(this->mist_program, "DepthTexture"), (int)GBuffer::Depth);
	glUniform1i(glGetUniformLocation(this->mist_program, "IrradianceTexture"), 2);

	glUseProgram(0);

	/*	*/
	GLint minMapBufferSize = 0;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
	this->uniformAlignSize = Math::align<size_t>(sizeof(MistUniformBuffer), (size_t)minMapBufferSize);

	/*	Create uniform buffer.	*/
	glGenBuffers(1, &this->uniform_buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
	glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignSize * 1, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenVertexArrays(1, &this->vao);
}

void MistPostProcessing::render(unsigned int skybox, unsigned int frame_texture, unsigned int depth_texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindVertexArray(this->vao);

	/*	Update uniform values.	*/
	glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
	void *uniformPointer =
		glMapBufferRange(GL_UNIFORM_BUFFER, 0 * this->uniformAlignSize,
						 this->uniformAlignSize, GL_MAP_WRITE_BIT);
	memcpy(uniformPointer, &this->mistsettings, sizeof(this->mistsettings));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
					  (1 % 1) * this->uniformAlignSize, this->uniformAlignSize);

	glUseProgram(this->mist_program);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + (int)GBuffer::Albedo);
	glBindTexture(GL_TEXTURE_2D, frame_texture);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + (int)GBuffer::Depth);
	glBindTexture(GL_TEXTURE_2D, depth_texture);

	/*	*/
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, skybox);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);
}