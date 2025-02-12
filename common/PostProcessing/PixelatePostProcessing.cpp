#include "PostProcessing/PixelatePostProcessing.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IOUtil.h>

using namespace glsample;

PixelatePostProcessing::PixelatePostProcessing() {
	this->setName("Pixelate");
	this->addRequireBuffer(GBuffer::Color);
}

PixelatePostProcessing::~PixelatePostProcessing() {
	if (this->pixelate_graphic_program >= 0) {
		glDeleteProgram(this->pixelate_graphic_program);
	}
}

void PixelatePostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *pixelate_frag_path = "Shaders/postprocessingeffects/pixelate.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->pixelate_graphic_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		/*	*/
		const std::vector<uint32_t> chromatic_abberation_fragment_binary =
			IOUtil::readFileData<uint32_t>(pixelate_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->pixelate_graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary,
																		  &chromatic_abberation_fragment_binary);
	}

	glUseProgram(this->pixelate_graphic_program);

	glUniform1i(glGetUniformLocation(this->pixelate_graphic_program, "ColorTexture"), (int)GBuffer::Albedo);
	glBindFragDataLocation(this->pixelate_graphic_program, 1, "fragColor");

	glUseProgram(0);

	this->vao = createVAO();
}

void PixelatePostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	const unsigned int source_texture = getMappedBuffer(GBuffer::Color);
	const unsigned int target_texture = getMappedBuffer(GBuffer::IntermediateTarget);

	glUseProgram(this->pixelate_graphic_program);

	glBindVertexArray(this->vao);
	/*	*/
	glUniform1f(glGetUniformLocation(this->pixelate_graphic_program, "settings.size"), this->settings.pixelSize);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);

	/*	Swap buffers.	*/
	framebuffer->attachments[0] = target_texture;
	framebuffer->attachments[1] = source_texture;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
}

void PixelatePostProcessing::renderUI() { ImGui::DragFloat("Pixel Size", &this->settings.pixelSize); }