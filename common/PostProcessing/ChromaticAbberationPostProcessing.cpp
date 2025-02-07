#include "PostProcessing/ChromaticAbberationPostProcessing.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <IOUtil.h>

using namespace glsample;

ChromaticAbberationPostProcessing::ChromaticAbberationPostProcessing() {
	this->setName("Chromatic Abberation");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
}

ChromaticAbberationPostProcessing::~ChromaticAbberationPostProcessing() {
	if (glIsProgram(this->chromatic_abberation_graphic_program)) {
		glDeleteProgram(this->chromatic_abberation_graphic_program);
	}
}

void ChromaticAbberationPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *chromatic_abberation_frag_path = "Shaders/postprocessingeffects/chromaticabbrevation.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->chromatic_abberation_graphic_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		/*	*/
		const std::vector<uint32_t> chromatic_abberation_fragment_binary =
			IOUtil::readFileData<uint32_t>(chromatic_abberation_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->chromatic_abberation_graphic_program = ShaderLoader::loadGraphicProgram(
			compilerOptions, &post_vertex_binary, &chromatic_abberation_fragment_binary);
	}

	glUseProgram(this->chromatic_abberation_graphic_program);

	glUniform1i(glGetUniformLocation(this->chromatic_abberation_graphic_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->chromatic_abberation_graphic_program, 1, "fragColor");
	glUseProgram(0);

	setItensity(1);

	this->vao = createVAO();
}

void ChromaticAbberationPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->render(framebuffer, this->getMappedBuffer(GBuffer::Color));
}

void ChromaticAbberationPostProcessing::setItensity(const float intensity) {
	glUseProgram(this->chromatic_abberation_graphic_program);
	glUniform1f(glGetUniformLocation(this->chromatic_abberation_graphic_program, "settings.blend"),
				this->getIntensity());
	glUseProgram(0);
}

void ChromaticAbberationPostProcessing::render(glsample::FrameBuffer *framebuffer, unsigned int source_color_texture) {

	unsigned int source_texture = source_color_texture;
	unsigned int target_texture = this->getMappedBuffer(GBuffer::IntermediateTarget);

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	glUseProgram(this->chromatic_abberation_graphic_program);

	glBindVertexArray(this->vao);

	/*	*/
	glUniform1f(glGetUniformLocation(this->chromatic_abberation_graphic_program, "settings.redOffset"),
				this->settings.redOffset);
	glUniform1f(glGetUniformLocation(this->chromatic_abberation_graphic_program, "settings.greenOffset"),
				this->settings.greenOffset);
	glUniform1f(glGetUniformLocation(this->chromatic_abberation_graphic_program, "settings.blueOffset"),
				this->settings.blueOffset);
	glUniform2f(glGetUniformLocation(this->chromatic_abberation_graphic_program, "settings.direction_center"),
				this->settings.direction_center[0], this->settings.direction_center[1]);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);

	/*	Swap buffers.	(ping pong)	*/
	std::swap(framebuffer->attachments[0], framebuffer->attachments[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[0], 0);

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
}

void ChromaticAbberationPostProcessing::renderUI() {
	ImGui::DragFloat("Red Offset", &this->settings.redOffset);
	ImGui::DragFloat("Green Offset", &this->settings.greenOffset);
	ImGui::DragFloat("Blue Offset", &this->settings.blueOffset);
	ImGui::DragFloat2("Center Offset", &this->settings.direction_center[0]);
}