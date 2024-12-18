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
	class SubGroup : public GLSampleWindow {
	  public:
		SubGroup() : GLSampleWindow() { this->setTitle("SubGroup - Compute"); }

		/*	Framebuffers.	*/
		unsigned int gameoflife_framebuffer{};
		std::vector<unsigned int> gameoflife_state_texture; /*	*/
		unsigned int gameoflife_render_texture{}; /*	No round robin required, since once updated, it is instantly
												   blitted. thus no implicit sync between frames.	*/
		size_t gameoflife_texture_width{};
		size_t gameoflife_texture_height{};

		/*	*/
		unsigned int gameoflife_program{};
		int localWorkGroupSize[3]{};

		unsigned int nthTexture = 0;

		/*	*/
		const std::string computeShaderPath = "Shaders/gameoflife/gameoflife.comp.spv";

		void Release() override {
			glDeleteProgram(this->gameoflife_program);

			glDeleteFramebuffers(1, &this->gameoflife_framebuffer);

			glDeleteTextures(this->gameoflife_state_texture.size(),
							 (const GLuint *)this->gameoflife_state_texture.data());
			glDeleteTextures(1, &this->gameoflife_render_texture);
		}

		void Initialize() override {

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> gameoflife_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->gameoflife_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &gameoflife_compute_binary);
			}

			/*	Setup compute pipeline.	*/
			glUseProgram(this->gameoflife_program);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "previousCellsTexture"), 0);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "currentCellsTexture"), 1);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "renderTexture"), 2);
			glGetProgramiv(this->gameoflife_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			{
				/*	Create framebuffer and its textures.	*/
				glGenFramebuffers(1, &this->gameoflife_framebuffer);
				this->gameoflife_state_texture.resize(3);
				glGenTextures(this->gameoflife_state_texture.size(), this->gameoflife_state_texture.data());
				glGenTextures(1, &this->gameoflife_render_texture);
				/*	Create init framebuffers.	*/
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->gameoflife_texture_width = width;
			this->gameoflife_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->gameoflife_framebuffer);

			std::vector<uint8_t> textureData(this->gameoflife_texture_width * this->gameoflife_texture_height *
											 sizeof(uint8_t));

			/*	Generate random game state.	*/
			for (size_t j = 0; j < this->gameoflife_texture_height; j++) {
				for (size_t i = 0; i < this->gameoflife_texture_width; i++) {
					/*	Random value between dead and alive cells.	*/
					textureData[this->gameoflife_texture_width * j + i] = Random::range(0, 2);
				}
			}

			/*	Create game of life state textures.	*/
			for (size_t i = 0; i < this->gameoflife_state_texture.size(); i++) {
				glBindTexture(GL_TEXTURE_2D, this->gameoflife_state_texture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, this->gameoflife_texture_width, this->gameoflife_texture_height,
							 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, textureData.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Create render target texture to show the result.	*/
			glBindTexture(GL_TEXTURE_2D, this->gameoflife_render_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->gameoflife_texture_width, this->gameoflife_texture_height, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	*/
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gameoflife_render_texture,
								   0);

			const GLenum drawAttach = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &drawAttach);

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
		}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());

			/*	Bind and Compute Game of Life Compute Program.	*/
			{
				glUseProgram(this->gameoflife_program);

				/*	Previous game of life state.	*/
				glBindImageTexture(
					0, this->gameoflife_state_texture[this->nthTexture % this->gameoflife_state_texture.size()], 0,
					GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
				/*	The resulting game of life state.	*/
				glBindImageTexture(
					1, this->gameoflife_state_texture[(this->nthTexture + 1) % this->gameoflife_state_texture.size()],
					0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

				/*	The image where the graphic version will be stored as.	*/
				glBindImageTexture(2, this->gameoflife_render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(this->gameoflife_texture_width / (float)this->localWorkGroupSize[0]),
								  std::ceil(this->gameoflife_texture_height / (float)this->localWorkGroupSize[1]), 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			/*	*/
			glViewport(0, 0, width, height);

			/*	Blit game of life render framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->gameoflife_framebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			/*	Blit with nearset to retain the details of each of the cells states.	*/
			glBlitFramebuffer(0, 0, this->gameoflife_texture_width, this->gameoflife_texture_height, 0, 0, width,
							  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			/*	Update the next frame in round robin.	*/
			this->nthTexture = (this->nthTexture + 1) % this->gameoflife_state_texture.size();
		}
		void update() override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		const std::vector<const char *> required_extension = {"GL_KHR_shader_subgroup"};
		GLSample<glsample::SubGroup> sample;
		sample.run(argc, argv, required_extension);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
