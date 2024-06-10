#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ReactionDiffusion : public GLSampleWindow {
	  public:
		ReactionDiffusion() : GLSampleWindow() {
			this->setTitle("ReactionDiffusion - Compute");

			this->reactionDiffusionSettingComponent =
				std::make_shared<ReactionDiffusionSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->reactionDiffusionSettingComponent);
		}

		struct reaction_diffusion_param_t {
			float kernelA[4][4] = {{0.05, 0.2, 0.05, 0}, {0.2, -1, 0.2, 0}, {0.05, 0.2, 0.05, 0}, {0, 0, 0, 0}};
			float kernelB[4][4] = {{0.25, 0.5, 0.25, 0}, {0.5, -3, 0.5, 0}, {0.25, 0.5, 0.25, 0}, {0, 0, 0, 0}};

			/*	*/
			float feedRate = 0.055f;
			float killRate = .062f;
			float diffuseRateA = 1.0f;
			float diffuseRateB = .5f;
			float delta = 0.1f;
			float speed = 1.0f;
			float padding0;
			float padding1;

		} uniformBuffer;

		/*	Framebuffers.	*/
		unsigned int reactiondiffusion_framebuffer;

		std::vector<unsigned int> reactiondiffusion_buffer;
		unsigned int reactiondiffusion_render_texture;
		size_t reactiondiffusion_texture_width;
		size_t reactiondiffusion_texture_height;

		int localWorkGroupSize[3];
		unsigned int reactiondiffusion_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 3;
		unsigned int current_cells_buffer_binding = 0;
		unsigned int previous_cells_buffer_binding = 1;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniformBuffer);

		unsigned int nthTexture = 0;

		class ReactionDiffusionSettingComponent : public nekomimi::UIComponent {
		  public:
			ReactionDiffusionSettingComponent(struct reaction_diffusion_param_t &uniform) : uniform(uniform) {
				this->setName("Reaction Diffusion Settings");
			}

			void draw() override {
				ImGui::DragFloat("Speed", &this->uniform.speed);
				ImGui::DragFloat("Feed Rate", &this->uniform.feedRate, 0.01f, 0.0f);
				ImGui::DragFloat("Kill Rate", &this->uniform.killRate, 0.01f, 0.0f);

				ImGui::DragFloat("Diffuse A", &this->uniform.diffuseRateA, 0.01f, 0.0f);
				ImGui::DragFloat("Diffuse B", &this->uniform.diffuseRateB, 0.01f, 0.0f);
			}

		  private:
			struct reaction_diffusion_param_t &uniform;
		};
		std::shared_ptr<ReactionDiffusionSettingComponent> reactionDiffusionSettingComponent;

		/*	*/
		const std::string computeShaderPath = "Shaders/reactiondiffusion/reactiondiffusion.comp.spv";

		void Release() override {
			glDeleteProgram(this->reactiondiffusion_program);

			glDeleteFramebuffers(1, &this->reactiondiffusion_framebuffer);

			glDeleteTextures(this->reactiondiffusion_buffer.size(),
							 (const GLuint *)this->reactiondiffusion_buffer.data());
			glDeleteTextures(1, &this->reactiondiffusion_render_texture);
		}

		void Initialize() override {

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> reactiondiffusion_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->reactiondiffusion_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &reactiondiffusion_compute_binary);
			}

			/*	Setup compute pipeline.	*/
			glUseProgram(this->reactiondiffusion_program);
			glUniform1i(glGetUniformLocation(this->reactiondiffusion_program, "renderTexture"), 2);

			glGetProgramiv(this->reactiondiffusion_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			int uniform_buffer_index = glGetUniformBlockIndex(this->reactiondiffusion_program, "UniformBufferBlock");
			glUniformBlockBinding(this->reactiondiffusion_program, uniform_buffer_index, this->uniform_buffer_binding);

			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	*/
				glGenFramebuffers(1, &this->reactiondiffusion_framebuffer);
				this->reactiondiffusion_buffer.resize(2);
				glGenBuffers(this->reactiondiffusion_buffer.size(), this->reactiondiffusion_buffer.data());
				glGenTextures(1, &this->reactiondiffusion_render_texture);

				/*	Create init framebuffers.	*/
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->reactiondiffusion_texture_width = width;
			this->reactiondiffusion_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->reactiondiffusion_framebuffer);

			std::vector<glm::vec2> textureData(this->reactiondiffusion_texture_width *
											   this->reactiondiffusion_texture_height);

			/*	Generate random game state.	*/
			for (size_t j = 0; j < this->reactiondiffusion_texture_height; j++) {
				for (size_t i = 0; i < this->reactiondiffusion_texture_width; i++) {

					/*	Normalized coordinate.	*/

					/*	*/
					textureData[this->reactiondiffusion_texture_width * j + i] =
						glm::vec2(fragcore::Math::PerlinNoise((float)i * 0.05f + 0, (float)j * 0.05f + 0, 1) * 1.0f,
								  fragcore::Math::PerlinNoise((float)i * 0.05f + 1, (float)j * 0.05f + 1, 1) * 1.0f);
				}
			}

			/*	Create game of life state textures.	*/
			for (size_t i = 0; i < this->reactiondiffusion_buffer.size(); i++) {
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->reactiondiffusion_buffer[i]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, textureData.size() * sizeof(textureData[0]), textureData.data(),
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			}

			/*	Create render target texture to show the result.	*/
			glBindTexture(GL_TEXTURE_2D, this->reactiondiffusion_render_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->reactiondiffusion_texture_width,
						 this->reactiondiffusion_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	*/
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
								   this->reactiondiffusion_render_texture, 0);

			const GLenum drawAttach = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawAttach);

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->reactiondiffusion_program);

				/*	Bind current cell state buffer.	*/
				glBindBufferBase(
					GL_SHADER_STORAGE_BUFFER, this->current_cells_buffer_binding,
					this->reactiondiffusion_buffer[this->nthTexture % this->reactiondiffusion_buffer.size()]);
				/*	Bind previous cell state buffer.	*/
				glBindBufferBase(
					GL_SHADER_STORAGE_BUFFER, this->previous_cells_buffer_binding,
					this->reactiondiffusion_buffer[(this->nthTexture + 1) % this->reactiondiffusion_buffer.size()]);

				/*	The image where the graphic version will be stored as.	*/
				glBindImageTexture(2, this->reactiondiffusion_render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
				glDispatchCompute(
					std::ceil(this->reactiondiffusion_texture_width / (float)this->localWorkGroupSize[0]),
					std::ceil(this->reactiondiffusion_texture_height / (float)this->localWorkGroupSize[1]), 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			/*	*/
			glViewport(0, 0, width, height);

			/*	Blit reaction diffusion render framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->reactiondiffusion_framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, this->reactiondiffusion_texture_width, this->reactiondiffusion_texture_height, 0, 0,
							  width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			/*	*/
			this->nthTexture = (this->nthTexture + 1) % this->reactiondiffusion_buffer.size();
		}

		void update() override {

			/*	*/
			this->uniformBuffer.delta = this->getTimer().deltaTime<float>();

			/*	Update uniform.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::ReactionDiffusion> sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
