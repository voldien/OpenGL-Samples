#include "MistPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <IOUtil.h>

using namespace glsample;

MistPostProcessing::MistPostProcessing() {
	this->setName("MistFog");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::Depth);
}

MistPostProcessing::~MistPostProcessing() {
	if (this->mist_program >= 0) {
		glDeleteProgram(this->mist_program);
	}
	if (glIsBuffer(this->uniform_buffer)) {
		glDeleteBuffers(1, &this->uniform_buffer);
	}
}

void MistPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *mist_fog_frag_path = "Shaders/postprocessingeffects/mistfog.frag.spv";
	const char *post_vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	if (this->mist_program == -1) {
		/*	*/
		const std::vector<uint32_t> vertex_mistfog_post_processing_binary =
			IOUtil::readFileData<uint32_t>(post_vertex_path, filesystem);

		const std::vector<uint32_t> fragment_mistfog_post_processing_binary =
			IOUtil::readFileData<uint32_t>(mist_fog_frag_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*  */
		this->mist_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_mistfog_post_processing_binary,
															  &fragment_mistfog_post_processing_binary);
	}

	glUseProgram(this->mist_program);

	glUniform1i(glGetUniformLocation(this->mist_program, "ColorTexture"), (int)GBuffer::Albedo);
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

void MistPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->render(0, this->getMappedBuffer(GBuffer::Albedo), this->getMappedBuffer(GBuffer::Depth));
}

void MistPostProcessing::render(unsigned int skybox, unsigned int frame_texture, unsigned int depth_texture) {

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	/*	Update uniform values.	*/
	glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
	void *uniformPointer =
		glMapBufferRange(GL_UNIFORM_BUFFER, 0 * this->uniformAlignSize, this->uniformAlignSize, GL_MAP_WRITE_BIT);
	memcpy(uniformPointer, &this->mistsettings, sizeof(this->mistsettings));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
					  (1 % 1) * this->uniformAlignSize, this->uniformAlignSize);

	/*	*/
	{
		glUseProgram(this->mist_program);

		/*	*/
		glActiveTexture(GL_TEXTURE0 + (int)GBuffer::Albedo);
		glBindTexture(GL_TEXTURE_2D, frame_texture);

		/*	*/
		glActiveTexture(GL_TEXTURE0 + (int)GBuffer::Depth);
		glBindTexture(GL_TEXTURE_2D, depth_texture);

		/*	*/
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, skybox);

		/*	*/
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glBindVertexArray(this->vao);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glUseProgram(0);
	}
}

void MistPostProcessing::renderUI() {
	ImGui::DragInt("Fog Type", (int *)&this->mistsettings.fogSettings.fogType);
	ImGui::ColorEdit4("Fog Color", &this->mistsettings.fogSettings.fogColor[0],
					  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::DragFloat("Fog Density", &this->mistsettings.fogSettings.fogDensity);
	ImGui::DragFloat("Fog Intensity", &this->mistsettings.fogSettings.fogIntensity);
	ImGui::DragFloat("Fog Start", &this->mistsettings.fogSettings.fogStart);
	ImGui::DragFloat("Fog End", &this->mistsettings.fogSettings.fogEnd);
}