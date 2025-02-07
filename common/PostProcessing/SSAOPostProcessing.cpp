#include "PostProcessing/SSAOPostProcessing.h"
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

SSAOPostProcessing::SSAOPostProcessing() {
	this->setName("Space Space Ambient Occlusion");
	this->addRequireBuffer(GBuffer::Color);
	this->addRequireBuffer(GBuffer::Depth);
	this->addRequireBuffer(GBuffer::Normal);
}

SSAOPostProcessing::~SSAOPostProcessing() {
	if (this->ssao_depth_only_program >= 0) {
		glDeleteProgram(this->ssao_depth_only_program);
	}
	if (this->ssao_depth_world_program >= 0) {
		glDeleteProgram(this->ssao_depth_world_program);
	}
	if (this->overlay_program >= 0) {
		glDeleteProgram(this->overlay_program);
	}
	if (this->downsample_compute_program >= 0) {
		glDeleteProgram(this->downsample_compute_program);
	}

	if (glIsBuffer(this->uniform_ssao_buffer)) {
		glDeleteBuffers(1, &this->uniform_ssao_buffer);
	}

	if (glIsTexture(this->random_texture)) {
		glDeleteTextures(1, &this->random_texture);
	}
}

void SSAOPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	/*	*/
	const std::string vertexSSAOShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const std::string fragmentSSAOShaderPath = "Shaders/postprocessingeffects/ssao_world_space.frag.spv";

	/*	*/
	const std::string vertexSSAODepthOnlyShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const std::string fragmentSSAODepthOnlyShaderPath = "Shaders/postprocessingeffects/ssao_depth_only.frag.spv";

	/*	*/
	const std::string vertexOverlayShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
	const std::string fragmentOverlayTextureShaderPath = "Shaders/postprocessingeffects/overlay.frag.spv";

	{
		fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
		compilerOptions.target = fragcore::ShaderLanguage::GLSL;
		compilerOptions.glslVersion = 330;

		/*	Load shader source.	*/
		const std::vector<uint32_t> vertex_ssao_binary =
			IOUtil::readFileData<uint32_t>(vertexSSAOShaderPath, filesystem);
		const std::vector<uint32_t> fragment_ssao_binary =
			IOUtil::readFileData<uint32_t>(fragmentSSAOShaderPath, filesystem);

		/*	Load shader source.	*/
		const std::vector<uint32_t> vertex_ssao_depth_only_binary =
			IOUtil::readFileData<uint32_t>(vertexSSAODepthOnlyShaderPath, filesystem);
		const std::vector<uint32_t> fragment_ssao_depth_onlysource =
			IOUtil::readFileData<uint32_t>(fragmentSSAODepthOnlyShaderPath, filesystem);

		/*	*/
		const std::vector<uint32_t> texture_vertex_binary =
			IOUtil::readFileData<uint32_t>(vertexOverlayShaderPath, filesystem);
		const std::vector<uint32_t> texture_fragment_binary =
			IOUtil::readFileData<uint32_t>(fragmentOverlayTextureShaderPath, filesystem);

		/*	Load shader	*/
		this->ssao_depth_world_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_ssao_binary, &fragment_ssao_binary);

		/*	Load shader	*/
		this->ssao_depth_only_program = ShaderLoader::loadGraphicProgram(
			compilerOptions, &vertex_ssao_depth_only_binary, &fragment_ssao_depth_onlysource);
		/*	Load shader	*/
		this->overlay_program =
			ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
	}

	/*	Setup graphic ambient occlusion pipeline.	*/
	glUseProgram(this->ssao_depth_world_program);
	int uniform_ssao_world_buffer_index = glGetUniformBlockIndex(this->ssao_depth_world_program, "UniformBufferBlock");
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "WorldTexture"), (int)GBuffer::WorldSpace);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "NormalTexture"), (int)GBuffer::Normal);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "DepthTexture"), (int)GBuffer::Depth);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "NormalRandomize"),
				   (int)GBuffer::TextureCoordinate);
	glUniformBlockBinding(this->ssao_depth_world_program, uniform_ssao_world_buffer_index,
						  this->uniform_ssao_buffer_binding);
	glUseProgram(0);

	/*	Setup graphic ambient occlusion pipeline.	*/
	glUseProgram(this->ssao_depth_only_program);
	int uniform_ssao_depth_buffer_index = glGetUniformBlockIndex(this->ssao_depth_only_program, "UniformBufferBlock");
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_only_program, "DepthTexture"), (int)GBuffer::Depth);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_only_program, "NormalRandomize"),
				   (int)GBuffer::TextureCoordinate);
	glUniformBlockBinding(this->ssao_depth_only_program, uniform_ssao_depth_buffer_index,
						  this->uniform_ssao_buffer_binding);
	glUseProgram(0);

	/*	Align uniform buffer in respect to driver requirement.	*/
	GLint minMapBufferSize = 0;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);

	this->uniformSSAOBufferAlignSize =
		fragcore::Math::align<size_t>(this->uniformSSAOBufferAlignSize, (size_t)minMapBufferSize);

	/*	FIXME: improve vectors.		*/
	{
		/*	Create random vector.	*/
		std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
		std::default_random_engine generator(time(nullptr));
		for (size_t i = 0; i < SSAOPostProcessing::maxKernels; i++) {

			/*	*/
			glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
							 randomFloats(generator));

			sample = glm::normalize(sample);
			sample *= randomFloats(generator);

			float scale = (float)i / static_cast<float>(maxKernels);
			scale = fragcore::Math::lerp(0.1f, 1.0f, scale * scale);

			sample *= scale;
			this->uniformStageBlockSSAO.kernel[i] = glm::vec4(sample, 0);
		}

		/*	*/
		glGenBuffers(1, &this->uniform_ssao_buffer);
		glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformSSAOBufferAlignSize * 1, &this->uniformStageBlockSSAO,
					 GL_DYNAMIC_DRAW);
		glBindBufferARB(GL_UNIFORM_BUFFER, 0);

		/*	Create white texture.	*/
		this->white_texture = glsample::Common::createColorTexture(1, 1, Color::white());

		/*	Create noise normalMap.	*/
		const size_t noiseW = 4;
		const size_t noiseH = 4;
		std::vector<glm::vec3> ssaoRandomNoise(noiseW * noiseH);
		for (size_t noise_index = 0; noise_index < ssaoRandomNoise.size(); noise_index++) {
			ssaoRandomNoise[noise_index].r = randomFloats(generator) * 2.0 - 1.0;
			ssaoRandomNoise[noise_index].g = randomFloats(generator) * 2.0 - 1.0;
			ssaoRandomNoise[noise_index].b = 0.0f;
			/*	*/
			ssaoRandomNoise[noise_index] = glm::normalize(ssaoRandomNoise[noise_index]);
		}

		/*	Create random texture.	*/
		glGenTextures(1, &this->random_texture);
		glBindTexture(GL_TEXTURE_2D, this->random_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseW, noiseH, 0, GL_RGB, GL_FLOAT, ssaoRandomNoise.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		/*	Border clamped to max value, it makes the outside area.	*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

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

void SSAOPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {

	PostProcessing::draw(framebuffer, render_targets);
	this->render(framebuffer, 0, 0, 0);
}

void SSAOPostProcessing::render(glsample::FrameBuffer *framebuffer, unsigned int depth_texture,
								unsigned int world_texture, unsigned int normal_texture) {

	glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
	void *uniformPointer = glMapBufferRange(GL_UNIFORM_BUFFER, 0, this->uniformSSAOBufferAlignSize,
											GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	memcpy(uniformPointer, &this->uniformStageBlockSSAO, sizeof(uniformStageBlockSSAO));
	glUnmapBuffer(GL_UNIFORM_BUFFER);

	/*	*/
	glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

	/*	Draw Ambient Occlusion.	*/
	{
		glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer_binding, this->uniform_ssao_buffer, 0,
						  this->uniformSSAOBufferAlignSize);

		glActiveTexture(GL_TEXTURE0 + (int)GBuffer::TextureCoordinate);
		glBindTexture(GL_TEXTURE_2D, this->random_texture);

		glBindVertexArray(this->vao);

		glBindSampler((int)GBuffer::Depth, this->world_position_sampler);
		glBindSampler((int)GBuffer::TextureCoordinate, this->world_position_sampler);

		if (this->useDepthOnly) {

			glUseProgram((int)this->ssao_depth_only_program);

		} else {
			glUseProgram(this->ssao_depth_world_program);
		}
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
		glBindVertexArray(this->vao);
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

void SSAOPostProcessing::renderUI() {
	ImGui::DragFloat("Intensity", &uniformStageBlockSSAO.intensity, 0.1f, 0.0f);
	ImGui::DragFloat("Radius", &uniformStageBlockSSAO.radius, 0.35f, 0.0f);
	ImGui::DragInt("Sample", &uniformStageBlockSSAO.samples, 1, 0);
	ImGui::DragFloat("Bias", &uniformStageBlockSSAO.bias, 0.01f, 0, 1);
	ImGui::Checkbox("DownSample", &this->downScale);
	ImGui::Checkbox("Use Depth Only", &this->useDepthOnly);
}