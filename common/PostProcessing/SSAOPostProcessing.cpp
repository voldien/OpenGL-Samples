#include "PostProcessing/SSAOPostProcessing.h"
#include "Common.h"
#include "PostProcessing/PostProcessing.h"
#include "SampleHelper.h"
#include "ShaderLoader.h"
#include <IOUtil.h>
#include <random>

using namespace glsample;

SSAOPostProcessing::SSAOPostProcessing() { this->setName("Space Space Ambient Occlusion"); }
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
}

void SSAOPostProcessing::initialize(fragcore::IFileSystem *filesystem) {
	/*	*/
	const std::string vertexSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
	const std::string fragmentSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.frag.spv";

	/*	*/
	const std::string vertexSSAODepthOnlyShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
	const std::string fragmentSSAODepthOnlyShaderPath = "Shaders/ambientocclusion/ambientocclusion_depthonly.frag.spv";

	/*	*/
	const std::string vertexOverlayShaderPath = "Shaders/postprocessingeffects/overlay.vert.spv";
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
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "WorldTexture"), 1);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "NormalTexture"), 3);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "DepthTexture"), 4);
	glUniform1iARB(glGetUniformLocation(this->ssao_depth_world_program, "NormalRandomize"), 5);
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

	/*	*/
	glGenBuffers(1, &this->uniform_ssao_buffer);
	glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
	glBufferData(GL_UNIFORM_BUFFER, this->uniformSSAOBufferAlignSize * 1, &this->uniformStageBlockSSAO,
				 GL_DYNAMIC_DRAW);
	glBindBufferARB(GL_UNIFORM_BUFFER, 0);

	/*	FIXME: improve vectors.		*/
	{
		/*	Create random vector.	*/
		std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
		std::default_random_engine generator;
		for (size_t i = 0; i < maxKernels; ++i) {

			glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
							 randomFloats(generator));

			sample = glm::normalize(sample);
			sample *= randomFloats(generator);

			float scale = (float)i / static_cast<float>(maxKernels);
			scale = fragcore::Math::lerp(0.1f, 1.0f, scale * scale);

			sample *= scale;
			this->uniformStageBlockSSAO.kernel[i] = glm::vec4(sample, 0);
		}

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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseW, noiseH, 0, GL_RGBA, GL_FLOAT, ssaoRandomNoise.data());
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

	this->overlay_program = this->createOverlayGraphicProgram(filesystem);
	this->vao = createVAO();
}

void SSAOPostProcessing::draw(
	glsample::FrameBuffer *framebuffer,
	const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) {

	PostProcessing::draw(framebuffer, render_targets);

	{
		glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer_binding, this->uniform_ssao_buffer, 0,
						  this->uniformSSAOBufferAlignSize);

		glActiveTexture(GL_TEXTURE0 + (int)GBuffer::TextureCoordinate);
		glBindTexture(GL_TEXTURE_2D, this->random_texture);

		/*	*/
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

		glBindVertexArray(this->vao);

		glUseProgram(this->ssao_depth_only_program);

		/*	*/
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUseProgram(0);

		/*	Swap buffers.	(ping pong)	*/
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
		/*	*/
		std::swap(framebuffer->attachments[0], framebuffer->attachments[1]);
	}

	{
		glBindVertexArray(this->vao);
		glUseProgram(this->overlay_program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebuffer->attachments[0]);

		/*	Draw overlay.	*/
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_DST_COLOR);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUseProgram(0);

		glBindVertexArray(0);

		/*	Swap buffers.	(ping pong)	*/
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, framebuffer->attachments[1], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, framebuffer->attachments[0], 0);
		/*	*/
		std::swap(framebuffer->attachments[0], framebuffer->attachments[1]);
	}

	glBindVertexArray(0);
}
