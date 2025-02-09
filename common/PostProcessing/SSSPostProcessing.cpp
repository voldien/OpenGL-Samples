#include "PostProcessing/SSSPostProcessing.h"
#include "Common.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <IOUtil.h>
#include <random>

using namespace glsample;

SSSPostProcessing::SSSPostProcessing() {
	this->setName("Space Space Shadowing");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::Depth);
}
SSSPostProcessing::~SSSPostProcessing() {}

void SSSPostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	/*	*/
	const std::string vertexScreenSpaceShadowShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const std::string fragmentScreenSpaceShaderPath = "Shaders/postprocessingeffects/screen_space_shadowing.frag.spv";

	{
		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*	Load shader source.	*/
		const std::vector<uint32_t> vertex_ssao_depth_only_binary =
			IOUtil::readFileData<uint32_t>(vertexScreenSpaceShadowShaderPath, filesystem);
		const std::vector<uint32_t> fragment_ssao_depth_onlysource =
			IOUtil::readFileData<uint32_t>(fragmentScreenSpaceShaderPath, filesystem);

		/*	Load shader	*/
		this->screen_space_shadow_frag_program = ShaderLoader::loadGraphicProgram(
			compilerOptions, &vertex_ssao_depth_only_binary, &fragment_ssao_depth_onlysource);
	}

	/*	Setup graphic ambient occlusion pipeline.	*/
	glUseProgram(this->screen_space_shadow_frag_program);
	int uniform_sss_buffer_index = glGetUniformBlockIndex(this->screen_space_shadow_frag_program, "UniformBufferBlock");
	glUniform1iARB(glGetUniformLocation(this->screen_space_shadow_frag_program, "DepthTexture"), (int)GBuffer::Depth);
	glUniform1iARB(glGetUniformLocation(this->screen_space_shadow_frag_program, "ColorTexture"), (int)GBuffer::Albedo);
	// glUniformBlockBinding(this->screen_space_shadow_frag_program, uniform_sss_buffer_index, 0);
	glUseProgram(0);

	/*	Create sampler for sampling GBuffer regardless of the texture internal sampler.	*/
	glCreateSamplers(1, &this->world_position_sampler);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameterf(this->world_position_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MAX_LOD, 0);
	glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MIN_LOD, 0);

	/*	*/
	this->overlay_program = this->createOverlayGraphicProgram(filesystem);
	this->vao = createVAO();

	setItensity(1);
}

void SSSPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {

	PostProcessing::draw(framebuffer, render_targets);

	/*	*/
	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	/*	Draw Screen Space Shadowing.	*/
	{

		glBindVertexArray(this->vao);

		glBindSampler((int)GBuffer::Depth, this->world_position_sampler);
		glBindSampler((int)GBuffer::Albedo, this->world_position_sampler);

		glUseProgram((int)this->screen_space_shadow_frag_program);

		/*	*/
		glUniform1f(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.blend"),
					this->SSSSettings.blend);
		glUniform1i(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.g_sss_max_steps"),
					this->SSSSettings.g_sss_max_steps);
		glUniform1f(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.g_sss_ray_max_distance"),
					this->SSSSettings.g_sss_ray_max_distance);
		glUniform1f(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.g_sss_thickness"),
					this->SSSSettings.g_sss_thickness);
		glUniform1f(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.g_sss_step_length"),
					this->SSSSettings.g_sss_step_length);
		glUniform2fv(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.g_taa_jitter_offset"), 1,
					 &this->SSSSettings.g_taa_jitter_offset[0]);
		glUniform3fv(glGetUniformLocation(this->screen_space_shadow_frag_program, "settings.light_direction"), 1,
					 &this->SSSSettings.light_direction[0]);

		/*	*/
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUseProgram(0);

		glBindSampler((int)GBuffer::Depth, 0);
	}

	/*	*/
	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	/*	Overlay with the orignal color framebuffer.	*/
	{
		glUseProgram(this->overlay_program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer->attachments[1]);

		/*	Draw overlay.	*/
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT_AND_BACK);
		glDisable(GL_DEPTH_TEST);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUseProgram(0);
	}

	glBindVertexArray(0);
}

void SSSPostProcessing::renderUI() {
	ImGui::DragInt("Max Steps", (int *)&SSSSettings.g_sss_max_steps, 1, 0, 128);
	ImGui::DragFloat("Ray Max Distance", &SSSSettings.g_sss_ray_max_distance, 0.1f, 0.0f);
	ImGui::DragFloat("Tichkness", &SSSSettings.g_sss_thickness, 0.1f, 0.0f);
	ImGui::DragFloat("Step Length", &SSSSettings.g_sss_step_length, 0.1f, 0.0f);
	ImGui::DragFloat2("Jitter Offset", &SSSSettings.g_taa_jitter_offset[0], 0.1f, 0.0f);
	ImGui::DragFloat3("Light Position", &SSSSettings.light_direction[0], 0.1f, 0.0f);
}