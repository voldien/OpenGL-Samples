#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class ReactionDiffusion : public GLSampleWindow {
	  public:
		ReactionDiffusion() : GLSampleWindow() {
			this->setTitle("ReactionDiffusion - Compute");

			this->normalMapSettingComponent = std::make_shared<NormalMapSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->normalMapSettingComponent);
		}

		struct reaction_diffusion_param_t {
			float kernelA[4][4] = {{0.1, 0.5, 0.1, 0}, {0.5, -1, 0.5, 0}, {0.1, 0.5, 0.1, 0}, {0, 0, 0, 0}};
			float kernelB[4][4] = {{0.1, 0.5, 0.1, 0}, {0.5, -1, 0.5, 0}, {0.1, 0.5, 0.1, 0}, {0, 0, 0, 0}};
			float feedRate = 0.055f;
			float killRate = .062f;
			float diffuseRateA = 1.0f;
			float diffuseRateB = .5f;
			float delta = 10.1f;
			float speed = 1.0f;

		} uniformBuffer;

		/*	Framebuffers.	*/
		unsigned int reactiondiffusion_framebuffer;

		std::vector<unsigned int> reactiondiffusion_texture;
		unsigned int reactiondiffusion_render_texture;
		size_t reactiondiffusion_texture_width;
		size_t reactiondiffusion_texture_height;

		int localWorkGroupSize[3];
		unsigned int reactiondiffusion_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniformBuffer);

		unsigned int nthTexture = 0;

		class NormalMapSettingComponent : public nekomimi::UIComponent {
		  public:
			NormalMapSettingComponent(struct reaction_diffusion_param_t &uniform) : uniform(uniform) {
				this->setName("Reaction Diffusion Settings");
			}

			virtual void draw() override {
				// ImGui::ColorEdit4("Tint Color", &this->uniform.tintColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::TextUnformatted("Light Setting");
				// ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				// ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::TextUnformatted("Normal Setting");
				// ImGui::DragFloat("Strength", &this->uniform.normalStrength);
			}

		  private:
			struct reaction_diffusion_param_t &uniform;
		};
		std::shared_ptr<NormalMapSettingComponent> normalMapSettingComponent;

		/*	*/
		const std::string computeShaderPath = "Shaders/reactiondiffusion/reactiondiffusion.comp.spv";

		virtual void Release() override {
			glDeleteProgram(this->reactiondiffusion_program);

			glDeleteFramebuffers(1, &this->reactiondiffusion_framebuffer);

			glDeleteTextures(this->reactiondiffusion_texture.size(),
							 (const GLuint *)this->reactiondiffusion_texture.data());
			glDeleteTextures(1, &this->reactiondiffusion_render_texture);
		}

		virtual void Initialize() override {

			/*	Load shader binaries.	*/
			const std::vector<uint32_t> gameoflife_source =
				IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

			/*	*/
			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			std::vector<char> gameoflife_source_T =
				fragcore::ShaderCompiler::convertSPIRV(gameoflife_source, compilerOptions);

			/*	Create compute pipeline.	*/
			this->reactiondiffusion_program = ShaderLoader::loadComputeProgram({&gameoflife_source_T});

			/*	Setup compute pipeline.	*/
			glUseProgram(this->reactiondiffusion_program);
			glUniform1i(glGetUniformLocation(this->reactiondiffusion_program, "previousCellsTexture"), 0);
			glUniform1i(glGetUniformLocation(this->reactiondiffusion_program, "currentCellsTexture"), 1);
			glUniform1i(glGetUniformLocation(this->reactiondiffusion_program, "renderTexture"), 2);
			glGetProgramiv(this->reactiondiffusion_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
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

			/*	*/
			glGenFramebuffers(1, &this->reactiondiffusion_framebuffer);
			this->reactiondiffusion_texture.resize(2);
			glGenTextures(this->reactiondiffusion_texture.size(), this->reactiondiffusion_texture.data());

			/*	Create init framebuffers.	*/
			this->onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {
			this->reactiondiffusion_texture_width = width;
			this->reactiondiffusion_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->reactiondiffusion_framebuffer);

			std::vector<float> textureData(this->reactiondiffusion_texture_width *
										   this->reactiondiffusion_texture_width);

			/*	Generate random game state.	*/
			Random random;
			for (size_t j = 0; j < this->reactiondiffusion_texture_height; j++) {
				for (size_t i = 0; i < this->reactiondiffusion_texture_width; i++) {
					textureData[this->reactiondiffusion_texture_width * j + i] = Random::range(0, 2);
				}
			}

			/*	Create game of life state textures.	*/
			for (size_t i = 0; i < this->reactiondiffusion_texture.size(); i++) {
				glBindTexture(GL_TEXTURE_2D, this->reactiondiffusion_texture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, this->reactiondiffusion_texture_width,
							 this->reactiondiffusion_texture_height, 0, GL_RED, GL_FLOAT, textureData.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Create render target texture to show the result.	*/
			glGenTextures(1, &this->reactiondiffusion_render_texture);
			glBindTexture(GL_TEXTURE_2D, this->reactiondiffusion_render_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->reactiondiffusion_texture_width,
						 this->reactiondiffusion_texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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

		virtual void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->reactiondiffusion_program);

				glBindImageTexture(
					0, this->reactiondiffusion_texture[this->nthTexture % this->reactiondiffusion_texture.size()], 0,
					GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
				glBindImageTexture(
					1, this->reactiondiffusion_texture[(this->nthTexture + 1) % this->reactiondiffusion_texture.size()],
					0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
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
			this->nthTexture = (this->nthTexture + 1) % this->reactiondiffusion_texture.size();
		}

		virtual void update() override {

			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();

			/*	*/
			this->uniformBuffer.delta = this->getTimer().deltaTime();

			/*	*/
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
