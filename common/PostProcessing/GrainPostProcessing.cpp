#include "PostProcessing/GrainPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>

using namespace glsample;

GrainPostProcessing::GrainPostProcessing() {
	this->setName("Grain");
	this->addRequireBuffer(GBuffer::Albedo);
}

GrainPostProcessing::~GrainPostProcessing() {
	if (this->grain_program >= 0) {
		glDeleteProgram(this->grain_program);
	}
}

void GrainPostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	const char *grain_frag_path = "Shaders/postprocessingeffects/grain.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->grain_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		/*	*/
		const std::vector<uint32_t> grain_fragment_binary = IOUtil::readFileData<uint32_t>(grain_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->grain_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &grain_fragment_binary);

		this->vao = createVAO();
	}

	glUseProgram(this->grain_program);
	glUniform1i(glGetUniformLocation(this->grain_program, "ColorTexture"), 0);

	glUseProgram(0);
}

void GrainPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->convert(0);
}

void GrainPostProcessing::convert(unsigned int texture) {

	glBindVertexArray(this->vao);

	glUseProgram(this->grain_program);

	/*	*/
	glUniform1f(glGetUniformLocation(this->grain_program, "settings.time"), 1);
	glUniform1f(glGetUniformLocation(this->grain_program, "settings.intensity"), 0.015f);
	glUniform1f(glGetUniformLocation(this->grain_program, "settings.speed"), 1);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);
}
