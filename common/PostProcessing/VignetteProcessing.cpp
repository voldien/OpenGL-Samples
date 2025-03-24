#include "PostProcessing/VignetteProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IOUtil.h>

using namespace glsample;

VignetteProcessing::VignetteProcessing() {
	this->setName("Vignette");
	this->addRequireBuffer(GBuffer::Albedo);
}

VignetteProcessing::~VignetteProcessing() {
	if (this->vignette_program >= 0) {
		glDeleteProgram(this->vignette_program);
	}
}

void VignetteProcessing::initialize(fragcore::IFileSystem *filesystem) {

	const char *vignette_frag_path = "Shaders/postprocessingeffects/vignette.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->vignette_program <= 0) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		/*	*/
		const std::vector<uint32_t> grain_fragment_binary =
			IOUtil::readFileData<uint32_t>(vignette_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->vignette_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &grain_fragment_binary);

		this->vao = this->createVAO();
	}

	glUseProgram(this->vignette_program);
	glUniform1i(glGetUniformLocation(this->vignette_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->vignette_program, 0, "fragColor");
	glUseProgram(0);

	this->setItensity(1);
}

void VignetteProcessing::setItensity(const float intensity) {
	PostProcessing::setItensity(intensity);
	glUseProgram(this->vignette_program);

	glUniform1f(glGetUniformLocation(this->vignette_program, "settings.base.blend"), this->getIntensity());
	glUseProgram(0);
}

void VignetteProcessing::draw(glsample::FrameBuffer *framebuffer,
							  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	glUseProgram(this->vignette_program);

	glUniform1f(glGetUniformLocation(this->vignette_program, "settings.extent"), this->extent);

	/*	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(this->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glUseProgram(0);

	glBindVertexArray(0);

	/*	Don't need to flip/swap framebuffer attachment. writes only ontop and not dependent on neightor pixel data.	*/
}

void VignetteProcessing::renderUI() { ImGui::DragFloat("Extent", &this->extent); }