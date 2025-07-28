#include "PostProcessing/FXAAPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IO/IOUtil.h>

using namespace glsample;

FXAAPostProcessing::FXAAPostProcessing() {
	this->setName("FXAA");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
}

FXAAPostProcessing::~FXAAPostProcessing() {
	if (this->fxaa_frag_program >= 0) {
		glDeleteProgram(this->fxaa_frag_program);
	}
}

void FXAAPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	/*	*/
	const char *fxaa_compute_path = "Shaders/postprocessingeffects/fxaa.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	/*	*/
	if (this->fxaa_frag_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		const std::vector<uint32_t> guassian_blur_compute_binary =
			IOUtil::readFileData<uint32_t>(fxaa_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->fxaa_frag_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &guassian_blur_compute_binary);

		this->vao = createVAO();
	}

	/*  */
	glUseProgram(this->fxaa_frag_program);

	// glGetProgramiv(this->fxaa_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->fxaa_frag_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->fxaa_frag_program, 0, "fragColor");
	glUseProgram(0);
}

void FXAAPostProcessing::draw(glsample::FrameBuffer *framebuffer,
							  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	/*	*/
	const unsigned int source_texture = this->getMappedBuffer(GBuffer::Color);
	const unsigned int target_texture = this->getMappedBuffer(GBuffer::IntermediateTarget);

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	glUseProgram(this->fxaa_frag_program);

	/*	*/
	glUniform1f(glGetUniformLocation(this->fxaa_frag_program, "settings.span_max"), this->span_max);

	glUniform1f(glGetUniformLocation(this->fxaa_frag_program, "settings.reduce_min"), this->reduce_min);
	glUniform1f(glGetUniformLocation(this->fxaa_frag_program, "settings.reduce_mul"), this->reduce_mul);
	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(this->vao);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	/*	Swap buffers.	(ping pong)	*/
	framebuffer->attachments[0] = target_texture;
	framebuffer->attachments[1] = source_texture;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
}

void FXAAPostProcessing::renderUI() {
	ImGui::DragFloat("Span Max", &this->span_max, 1, 0, 0, "%.6f");
	ImGui::DragFloat("Reduce Min", &this->reduce_min, 1, 0, 0, "%.6f");
	ImGui::DragFloat("Reduce Mul", &this->reduce_mul, 1, 0, 0, "%.6f");
}