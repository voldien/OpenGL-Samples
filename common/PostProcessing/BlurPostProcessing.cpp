#include "BlurPostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include "magic_enum.hpp"
#include <GL/glew.h>
#include <IOUtil.h>
#include <limits>

using namespace glsample;

BlurPostProcessing::BlurPostProcessing() {
	this->setName("Blur");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
}

BlurPostProcessing::~BlurPostProcessing() {
	if (glIsProgram(this->guassian_blur_vertical_compute_program)) {
		glDeleteProgram(this->guassian_blur_vertical_compute_program);
	}
	if (glIsProgram(this->guassian_blur_horizontal_compute_program)) {
		glDeleteProgram(this->guassian_blur_horizontal_compute_program);
	}
	if (glIsProgram(this->box_blur_compute_program)) {
		glDeleteProgram(this->box_blur_compute_program);
	}
}

void BlurPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	const char *guassian_vertical_blur_compute_path = "Shaders/postprocessingeffects/guassian_blur_vertical.comp.spv";
	const char *guassian_horizontal_blur_compute_path =
		"Shaders/postprocessingeffects/guassian_blur_horizontal.comp.spv";
	const char *box_blur_compute_path = "Shaders/postprocessingeffects/box_blur.comp.spv";

	if (this->guassian_blur_vertical_compute_program == 0) {

		const std::vector<uint32_t> guassian_blur_vertical_compute_binary =
			IOUtil::readFileData<uint32_t>(guassian_vertical_blur_compute_path, filesystem);

		const std::vector<uint32_t> guassian_blur_horizontal_compute_binary =
			IOUtil::readFileData<uint32_t>(guassian_horizontal_blur_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420; /*	Min required glsl spec.	*/

		/*  */
		this->guassian_blur_vertical_compute_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_vertical_compute_binary);
		/*  */
		this->guassian_blur_horizontal_compute_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &guassian_blur_horizontal_compute_binary);
	}

	if (this->box_blur_compute_program == 0) {
		/*	*/
		const std::vector<uint32_t> box_blur_compute_binary =
			IOUtil::readFileData<uint32_t>(box_blur_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420; /*	Min required glsl spec.	*/

		/*  */
		this->box_blur_compute_program = ShaderLoader::loadComputeProgram(compilerOptions, &box_blur_compute_binary);
	}

	glUseProgram(this->guassian_blur_vertical_compute_program);
	glGetProgramiv(this->guassian_blur_vertical_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "ColorTexture"), 0);
	glUniform1i(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "TargetTexture"), 1);
	glUseProgram(0);

	glUseProgram(this->guassian_blur_horizontal_compute_program);
	glGetProgramiv(this->guassian_blur_horizontal_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "ColorTexture"), 0);
	glUniform1i(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "TargetTexture"), 1);
	glUseProgram(0);

	glUseProgram(this->box_blur_compute_program);
	glGetProgramiv(this->box_blur_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->box_blur_compute_program, "ColorTexture"), 0);
	glUniform1i(glGetUniformLocation(this->box_blur_compute_program, "TargetTexture"), 1);
	glUseProgram(0);

	/*	Create sampler.	*/
	glCreateSamplers(1, &this->texture_sampler);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameterf(this->texture_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MAX_LOD, 0);
	glSamplerParameteri(this->texture_sampler, GL_TEXTURE_MIN_LOD, 0);

	/*	Update Guassian */
	updateGuassianKernel();

	setItensity(1);
}

void BlurPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);
	
	this->render(framebuffer, this->getMappedBuffer(GBuffer::Color),
				 this->getMappedBuffer(GBuffer::IntermediateTarget));
}

void BlurPostProcessing::render(glsample::FrameBuffer *framebuffer, unsigned int read_color_texture,
								unsigned int write_texture) {
	GLint width = 0;
	GLint height = 0;

	/*	*/
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, write_texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	switch (this->blurType) {
	case BoxBlur:
		glUseProgram(this->box_blur_compute_program);
		glUniform1f(glGetUniformLocation(this->box_blur_compute_program, "settings.variance"), this->variance);
		glUniform1f(glGetUniformLocation(this->box_blur_compute_program, "settings.mean"), this->mean);
		glUniform1f(glGetUniformLocation(this->box_blur_compute_program, "settings.radius"), this->radius);
		glUniform1i(glGetUniformLocation(this->box_blur_compute_program, "settings.samples"), this->samples);
		break;
	case GuassianBlur:
		glUseProgram(this->guassian_blur_vertical_compute_program);
		glUniform1f(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "settings.variance"),
					this->variance);
		glUniform1f(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "settings.mean"), this->mean);
		glUniform1f(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "settings.radius"),
					this->radius);
		glUniform1i(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "settings.samples"),
					this->samples);
		glUseProgram(this->guassian_blur_horizontal_compute_program);
		glUniform1f(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "settings.variance"),
					this->variance);
		glUniform1f(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "settings.mean"), this->mean);
		glUniform1f(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "settings.radius"),
					this->radius);
		glUniform1i(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "settings.samples"),
					this->samples);
		break;
	default:
	case MaxBlur:
		break;
	}

	glBindSampler(0, this->texture_sampler);

	const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]);
	const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]);

	switch (this->blurType) {
	case BoxBlur: {
		glUseProgram(this->box_blur_compute_program);
		if (WorkGroupX > 0 && WorkGroupY > 0) {
			for (int it = 0; it < this->nrIterations; it++) {

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, read_color_texture);
				glBindImageTexture(1, write_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(WorkGroupX, WorkGroupY, 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				/*	Swap buffers.	(ping pong)	*/
				std::swap(write_texture, read_color_texture);
			}
		}
	} break;
	case GuassianBlur: {

		if (WorkGroupX > 0 && WorkGroupY > 0) {
			for (int it = 0; it < this->nrIterations; it++) {

				glUseProgram(this->guassian_blur_vertical_compute_program);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, read_color_texture);
				glBindImageTexture(1, write_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(WorkGroupX, WorkGroupY, 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				std::swap(write_texture, read_color_texture);

				glUseProgram(this->guassian_blur_horizontal_compute_program);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, read_color_texture);
				glBindImageTexture(1, write_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glDispatchCompute(WorkGroupX, WorkGroupY, 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

				std::swap(write_texture, read_color_texture);
			}
		}
	} break;
	default:
	case MaxBlur:
		break;
	}

	glUseProgram(0);

	/*	Swap buffers.	(ping pong)	*/
	std::swap(write_texture, read_color_texture);

	/*	Reset framebuffer attachment placements.	*/
	framebuffer->attachments[0] = write_texture;
	framebuffer->attachments[1] = read_color_texture;
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[0], 0);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
}

void BlurPostProcessing::updateGuassianKernel() {
	Math::guassian<float>(this->guassian.data(), this->samples, 0, this->variance);

	glUseProgram(this->guassian_blur_vertical_compute_program);
	glUniform1fv(glGetUniformLocation(this->guassian_blur_vertical_compute_program, "settings.kernel"), this->samples,
				 this->guassian.data());
	glUseProgram(0);

	glUseProgram(this->guassian_blur_horizontal_compute_program);
	glUniform1fv(glGetUniformLocation(this->guassian_blur_horizontal_compute_program, "settings.kernel"), this->samples,
				 this->guassian.data());
	glUseProgram(0);
}

void BlurPostProcessing::renderUI() {

	if (ImGui::DragFloat("Variance", &this->variance, 1.0f, 0.0, std::numeric_limits<float>::max())) {
		/*	Update Guassian */
		this->updateGuassianKernel();
	}
	ImGui::DragFloat("Radius", &this->radius);
	if (ImGui::DragInt("Samples", &this->samples, 1.0, 1, maxSamples)) {
		/*	Update Guassian */
		this->updateGuassianKernel();
	}
	if (ImGui::DragInt("Number Iterations", &this->nrIterations, 1.0f, 0, std::numeric_limits<int>::max())) {
	}

	const int item_selected_idx = (int)this->blurType; // Here we store our selection data as an index.

	std::string combo_preview_value = std::string(magic_enum::enum_name(this->blurType));
	ImGuiComboFlags flags = 0;
	if (ImGui::BeginCombo("Blur Type", combo_preview_value.c_str(), flags)) {
		for (int n = 0; n < (int)Blur::MaxBlur; n++) {
			const bool is_selected = (item_selected_idx == n);

			if (ImGui::Selectable(magic_enum::enum_name((Blur)n).data(), is_selected)) {
				this->blurType = (Blur)n;
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}