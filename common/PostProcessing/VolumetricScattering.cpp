#include "PostProcessing/VolumetricScattering.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IOUtil.h>

using namespace glsample;

VolumetricScatteringPostProcessing::VolumetricScatteringPostProcessing() {
	this->setName("VolumetriccScattering");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::Depth);
}

VolumetricScatteringPostProcessing::~VolumetricScatteringPostProcessing() {

	if (this->volumetric_scattering_legacy_program >= 0) {
		glDeleteProgram(this->volumetric_scattering_legacy_program);
	}

	if (this->volumetric_scattering_raymarching_shadow_depth_program >= 0) {
		glDeleteProgram(this->volumetric_scattering_raymarching_shadow_depth_program);
	}

	if (this->downsample_compute_program >= 0) {
		glDeleteProgram(this->downsample_compute_program);
	}
}

void VolumetricScatteringPostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	/*	*/
	const std::string vertexSSAOShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const std::string fragmentVolumetricLegacyShaderPath =
		"Shaders/postprocessingeffects/volumetric_scattering_legacy.frag.spv";
	const std::string fragmentVolumetricRayMarchingShaderPath =
		"Shaders/postprocessingeffects/volumetric_scattering_raymarching_shadowmap.frag.spv";

	{
		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*	Load shader source.	*/
		const std::vector<uint32_t> vertex_post_binary =
			IOUtil::readFileData<uint32_t>(vertexSSAOShaderPath, filesystem);
		const std::vector<uint32_t> fragment_volumetric_legacy_binary =
			IOUtil::readFileData<uint32_t>(fragmentVolumetricLegacyShaderPath, filesystem);

		const std::vector<uint32_t> fragment_volumetric_raymarching_binary =
			IOUtil::readFileData<uint32_t>(fragmentVolumetricRayMarchingShaderPath, filesystem);

		/*	Load shader	*/
		this->volumetric_scattering_legacy_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_post_binary, &fragment_volumetric_legacy_binary);

		/*	Load shader	*/
		this->volumetric_scattering_raymarching_shadow_depth_program = ShaderLoader::loadGraphicProgram(
			compilerOptions, &vertex_post_binary, &fragment_volumetric_raymarching_binary);
	}

	/*	Setup graphic Volumetric Scattering Legacy pipeline.	*/
	{
		glUseProgram(this->volumetric_scattering_legacy_program);
		glUniform1iARB(glGetUniformLocation(this->volumetric_scattering_legacy_program, "ColorTexture"),
					   (int)GBuffer::Albedo);
		glUniform1iARB(glGetUniformLocation(this->volumetric_scattering_legacy_program, "DepthTexture"),
					   (int)GBuffer::Depth);
		glBindFragDataLocation(this->volumetric_scattering_legacy_program, 1, "fragColor");
		glUseProgram(0);
	}

	/*	Setup graphic Volumetric Scattering RayMarching	pipeline.	*/
	{
		glUseProgram(this->volumetric_scattering_raymarching_shadow_depth_program);
		glUniform1iARB(glGetUniformLocation(this->volumetric_scattering_raymarching_shadow_depth_program, "ColorTexture"),
					   (int)GBuffer::Albedo);
		glUniform1iARB(glGetUniformLocation(this->volumetric_scattering_raymarching_shadow_depth_program, "DepthTexture"),
					   (int)GBuffer::Depth);
		glUniform1iARB(
			glGetUniformLocation(this->volumetric_scattering_raymarching_shadow_depth_program, "DirectionalShadowTexture"),
			(int)GBuffer::Depth);
		glBindFragDataLocation(this->volumetric_scattering_raymarching_shadow_depth_program, 1, "fragColor");
		glUseProgram(0);
	}

	/*	Create sampler.	*/
	glCreateSamplers(1, &this->texture_sampler);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameterf(this->texture_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAX_LOD, 0);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_LOD, 0);

	this->vao = createVAO();
}

void VolumetricScatteringPostProcessing::setItensity(const float intensity) {
	PostProcessing::setItensity(intensity);
	glUseProgram(this->volumetric_scattering_legacy_program);

	glUniform1f(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings.base.blend"),
				this->getIntensity());
	glUseProgram(0);
}

void VolumetricScatteringPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) {

	PostProcessing::draw(framebuffer, render_targets);

	const unsigned int source_texture = this->getMappedBuffer(GBuffer::Color);
	const unsigned int target_texture = this->getMappedBuffer(GBuffer::IntermediateTarget);

	glBindSampler((int)GBuffer::Albedo, texture_sampler);
	glBindSampler((int)GBuffer::Depth, texture_sampler);

	{
		glBindVertexArray(this->vao);
		glUseProgram(this->volumetric_scattering_legacy_program);

		/*	*/
		glUniform1i(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings.numSamples"),
					this->volumetricScatteringSettings.numSamples);
		glUniform1f(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings._Density"),
					this->volumetricScatteringSettings.Density);
		glUniform1f(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings._Decay"),
					this->volumetricScatteringSettings.Decay);
		glUniform1f(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings._Weight"),
					this->volumetricScatteringSettings.Weight);
		glUniform1f(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings._Exposure"),
					this->volumetricScatteringSettings.Exposure);
		glUniform2fv(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings.lightPosition"), 1,
					 &this->volumetricScatteringSettings.lightPosition[0]);
		glUniform4fv(glGetUniformLocation(this->volumetric_scattering_legacy_program, "settings.color"), 1,
					 &this->volumetricScatteringSettings.color[0]);

		/*	*/
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUseProgram(0);

		glBindVertexArray(0);

		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		/*	Swap buffers.	*/
		framebuffer->attachments[0] = target_texture;
		framebuffer->attachments[1] = source_texture;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
	}
}

void VolumetricScatteringPostProcessing::renderUI() {

	// TODO: add option to choose

	ImGui::DragInt("Samples", (int *)&volumetricScatteringSettings.numSamples, 1, 0, 128);
	ImGui::DragFloat("Decay", &volumetricScatteringSettings.Decay, 0.1f, 0.0f);
	ImGui::DragFloat("Density", &volumetricScatteringSettings.Density, 0.1f, 0.0f);
	ImGui::DragFloat("Exposure", &volumetricScatteringSettings.Exposure, 0.1f, 0.0f);
	ImGui::DragFloat("Weight", &volumetricScatteringSettings.Weight, 0.1f, 0.0f);
	ImGui::DragFloat2("Light Position", &volumetricScatteringSettings.lightPosition[0], 0.1f, 0.0f);
	ImGui::ColorEdit4("Color", &this->volumetricScatteringSettings.color[0],
					  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

	//ImGui::RadioButton("Legacy", &this->volumetric_mode, 1);
	ImGui::SetItemTooltip("I am a tooltip");
	ImGui::SameLine();
	//ImGui::Button("Ray Tracing", &this->volumetric_mode, 2);
	ImGui::SetItemTooltip("I am a tooltip");

}