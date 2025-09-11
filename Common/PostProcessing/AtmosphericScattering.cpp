#include "PostProcessing/AtmosphericScattering.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <IO/IOUtil.h>

using namespace glsample;

AtmosphericScattering::AtmosphericScattering() {
	this->setName("Atmospheric Scattering");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::Depth);
}

AtmosphericScattering::~AtmosphericScattering() {
	if (this->atmospheric_scattering_graphic_program >= 0) {
		glDeleteProgram(this->atmospheric_scattering_graphic_program);
	}
	if (glIsSampler(this->texture_sampler)) {
		glDeleteSamplers(1, &this->texture_sampler);
	}
}

void AtmosphericScattering::initialize(fragcore::IFileSystem *filesystem) {

	const char *vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const char *glow_frag_path = "Shaders/postprocessingeffects/atmospheric_scattering.frag.spv";

	if (this->atmospheric_scattering_graphic_program == -1) {

		/*	*/
		const std::vector<uint32_t> post_vertex_binary =
			fragcore::IOUtil::readFileData<uint32_t>(vertex_path, filesystem); /*	*/
		const std::vector<uint32_t> glow_fragment_binary =
			fragcore::IOUtil::readFileData<uint32_t>(glow_frag_path, filesystem);
		/*	*/
		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420; /*	*/

		/*  */
		this->atmospheric_scattering_graphic_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &glow_fragment_binary);
	}

	/*	*/
	glUseProgram(this->atmospheric_scattering_graphic_program);
	glUniform1i(glGetUniformLocation(this->atmospheric_scattering_graphic_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->atmospheric_scattering_graphic_program, 1, "fragColor");
	glUseProgram(0);

	/*	Create sampler.	*/
	glCreateSamplers(1, &this->texture_sampler);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameterf(this->texture_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAX_LOD, 0);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_LOD, 0);

	this->overlay_program = this->createOverlayGraphicProgram(filesystem);

	this->vao = this->createVAO();

	setIntensity(1);
}

void AtmosphericScattering::draw(glsample::FrameBuffer *framebuffer,
								 const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->render(framebuffer, this->getMappedBuffer(GBuffer::Color));
}

void AtmosphericScattering::render(FrameBuffer *framebuffer, unsigned int color_texture) {

	if (this->settings.numSamples == 0) {
		return;
	}

	unsigned int intermediate0 = this->getMappedBuffer(GBuffer::IntermediateTarget);
	unsigned int intermediate1 = this->getMappedBuffer(GBuffer::IntermediateTarget2);

	unsigned int readTexture = color_texture; /*	*/
	unsigned int targetTexture = intermediate0;

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindSampler((int)GBuffer::Albedo, this->texture_sampler);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	{
		glUseProgram(this->overlay_program);

		/*	*/
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, readTexture);
		/*	*/
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		/*	Draw overlay.	*/
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		// glBlendColor(0, 0, 0, 1 - this->threadshold);
		glBlendFuncSeparate(GL_ONE_MINUS_CONSTANT_ALPHA, GL_ONE, GL_ONE, GL_ZERO);

		glBindVertexArray(this->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glUseProgram(0);
	}

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
}

void AtmosphericScattering::renderUI() {

	/*	Each planet.	*/
	for (size_t i = 0; i < 0; i++) {
	}

	ImGui::DragFloat2("Light Position", &settings.sunDirection[0], 0.1f, 0.0f);

	ImGui::DragInt("Image Size", (&this->settings.numSamples));
	ImGui::DragFloat("Hr", &this->settings.Hr, 1, 0, 1000.0f);
	ImGui::DragFloat("Hm", &this->settings.Hm, 1, 0, 1000.0f);
}