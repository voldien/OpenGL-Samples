#include "PostProcessing/BloomPostProcessing.h"
#include "GLSampleSession.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <IOUtil.h>

using namespace glsample;

BloomPostProcessing::BloomPostProcessing() {
	this->setName("Bloom");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::IntermediateTarget);
	this->addRequireBuffer(GBuffer::IntermediateTarget2);
}

BloomPostProcessing::~BloomPostProcessing() {
	if (this->bloom_blur_graphic_program >= 0) {
		glDeleteProgram(this->bloom_blur_graphic_program);
	}
	if (glIsSampler(this->texture_sampler)) {
		glDeleteSamplers(1, &this->texture_sampler);
	}
}

void BloomPostProcessing::initialize(fragcore::IFileSystem *filesystem) {

	const char *bloom_frag_path = "Shaders/postprocessingeffects/bloom.frag.spv";
	const char *vertex_path = "Shaders/postprocessingeffects/postprocessing.vert.spv";

	const char *downscale_compute_path = "Shaders/compute/downsample2x2.comp.spv";
	const char *upscale_compute_path = "Shaders/compute/upsample2x2.comp.spv";

	if (this->bloom_blur_graphic_program == -1) {
		/*	*/
		const std::vector<uint32_t> post_vertex_binary =
			IOUtil::readFileData<uint32_t>(vertex_path, filesystem); /*	*/
		const std::vector<uint32_t> guassian_blur_compute_binary =
			IOUtil::readFileData<uint32_t>(bloom_frag_path, filesystem);

		const std::vector<uint32_t> downsample2x2_compute_binary =
			IOUtil::readFileData<uint32_t>(downscale_compute_path, filesystem);

		const std::vector<uint32_t> upsample2x2_compute_binary =
			IOUtil::readFileData<uint32_t>(upscale_compute_path, filesystem);

		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 420;

		/*  */
		this->bloom_blur_graphic_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &post_vertex_binary, &guassian_blur_compute_binary);

		/*  */
		this->downsample_compute_program =
			ShaderLoader::loadComputeProgram(compilerOptions, &downsample2x2_compute_binary);

		/*  */
		this->upsample_compute_program = ShaderLoader::loadComputeProgram(compilerOptions, &upsample2x2_compute_binary);
	}

	glUseProgram(this->bloom_blur_graphic_program);
	glUniform1i(glGetUniformLocation(this->bloom_blur_graphic_program, "ColorTexture"), 0);
	glBindFragDataLocation(this->bloom_blur_graphic_program, 1, "fragColor");
	glUseProgram(0);

	glUseProgram(this->downsample_compute_program);
	glGetProgramiv(this->downsample_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->downsample_compute_program, "SourceTexture"), 0);
	glUniform1i(glGetUniformLocation(this->downsample_compute_program, "TargetTexture"), 1);
	glUseProgram(0);

	glUseProgram(this->upsample_compute_program);
	glGetProgramiv(this->upsample_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
	glUniform1i(glGetUniformLocation(this->upsample_compute_program, "SourceTexture"), 0);
	glUniform1i(glGetUniformLocation(this->upsample_compute_program, "TargetTexture"), 1);
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

	setItensity(1);
}

void BloomPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {
	PostProcessing::draw(framebuffer, render_targets);

	this->render(framebuffer, this->getMappedBuffer(GBuffer::Color));
}

void BloomPostProcessing::render(FrameBuffer *framebuffer, unsigned int color_texture) {
	if (this->nr_down_samples == 0) {
		return;
	}

	unsigned int intermediate0 = this->getMappedBuffer(GBuffer::IntermediateTarget);
	unsigned int intermediate1 = this->getMappedBuffer(GBuffer::IntermediateTarget2);

	unsigned int readTexture = color_texture; /*	*/
	unsigned int targetTexture = intermediate0;

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindSampler((int)GBuffer::Albedo, this->texture_sampler);

	{
		GLint width = 0;
		GLint height = 0;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer->attachments[0]);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		glUseProgram(this->downsample_compute_program);
		glUniform1i(glGetUniformLocation(this->downsample_compute_program, "settings.filterRadius"), 1);

		for (size_t i = 0; i < nr_down_samples; i++) {

			/*	Swap away from original texture color and use intermediate texture only.	*/
			if (i == 1) {
				readTexture = intermediate0;
				targetTexture = intermediate1;
			}

			const unsigned int WorkGroupX = std::ceil(width / (float)localWorkGroupSize[0]) / (1 << (i + 1));
			const unsigned int WorkGroupY = std::ceil(height / (float)localWorkGroupSize[1]) / (1 << (i + 1));

			if (WorkGroupX >= 2 && WorkGroupY >= 2) {

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, readTexture);
				glBindImageTexture(1, targetTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				glDispatchCompute(WorkGroupX, WorkGroupY, 1);

				std::swap(readTexture, targetTexture);
			}
		}

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/*	*/
		glUseProgram(this->upsample_compute_program);
		glUniform1i(glGetUniformLocation(this->upsample_compute_program, "settings.filterRadius"), 1);

		for (size_t i = 0; i < nr_down_samples; i++) {

			const unsigned int WorkGroupX =
				std::ceil(width / (float)localWorkGroupSize[0]) / (1 << (nr_down_samples - i - 1));
			const unsigned int WorkGroupY =
				std::ceil(height / (float)localWorkGroupSize[1]) / (1 << (nr_down_samples - i - 1));

			if (WorkGroupX >= 2 && WorkGroupY >= 2) {

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, readTexture);

				glBindImageTexture(1, targetTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				glDispatchCompute(WorkGroupX, WorkGroupY, 1);
				std::swap(readTexture, targetTexture);
			}
		}

		glBindSampler(0, 0);
	}

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
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ZERO);

		glBindVertexArray(this->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		glUseProgram(0);
	}

	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
}

void BloomPostProcessing::renderUI() { ImGui::DragInt("Image Size", (int *)&this->nr_down_samples); }