#include "PostProcessing/GrainPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IO/IOUtil.h>
#include <SDL_opengl.h>

using namespace glsample;

GrainPostProcessing::GrainPostProcessing() {
	this->setName("Grain");
	this->addRequireBuffer(GBuffer::Albedo);
}

GrainPostProcessing::~GrainPostProcessing() {
	if (this->grain_graphic_program >= 0) {
		glDeleteProgram(this->grain_graphic_program);
	}
}

void GrainPostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	const char *grain_frag_path = "Shaders/postprocessingeffects/grain.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->grain_graphic_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary = IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);
		/*	*/
		const std::vector<uint32_t> grain_fragment_binary = IOUtil::readFileData<uint32_t>(grain_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->grain_graphic_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &grain_fragment_binary);

		this->vao = createVAO();
	}

	glUseProgram(this->grain_graphic_program);
	glUniform1i(glGetUniformLocation(this->grain_graphic_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->grain_graphic_program, 0, "fragColor");
	glUseProgram(0);
}

void GrainPostProcessing::setIntensity(const float intensity) {
	PostProcessing::setIntensity(intensity);
	glUniform1f(glGetUniformLocation(this->grain_graphic_program, "settings.base.blend"), this->getIntensity());
}

void GrainPostProcessing::draw(glsample::FrameBuffer *framebuffer,
							   const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {

	PostProcessing::draw(framebuffer, render_targets);

	glUseProgram(this->grain_graphic_program);

	if (animate) {
		grainSettings.time += 0.01f;
	}

	/*	*/
	glUniform1f(glGetUniformLocation(this->grain_graphic_program, "settings.base.blend"), this->getIntensity());
	glUniform1f(glGetUniformLocation(this->grain_graphic_program, "settings.time"), grainSettings.time);
	glUniform1f(glGetUniformLocation(this->grain_graphic_program, "settings.intensity"), grainSettings.intensity);
	glUniform1f(glGetUniformLocation(this->grain_graphic_program, "settings.speed"), grainSettings.speed);

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

void GrainPostProcessing::renderUI() {
	ImGui::DragFloat("Time", &this->grainSettings.time);
	ImGui::DragFloat("Intensity Strength", &this->grainSettings.intensity);

	this->animate = ImGui::Checkbox("Animiate", &this->animate);
	ImGui::DragFloat("Speed", &this->grainSettings.speed);
	ImGui::DragFloat("Scale", &this->grainSettings.scale);
}