#include "GLSampleWindow.h"
#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <ShaderCompiler.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class GameOfLife : public GLSampleWindow {
	  public:
		GameOfLife() : GLSampleWindow() { this->setTitle("GameOfLife - Compute"); }

		/*	*/
		unsigned int gameoflife_framebuffer;
		unsigned int gameoflife_program;
		std::vector<unsigned int> gameoflife_texture;
		unsigned int gameoflife_render_texture;
		size_t gameoflife_texture_width;
		size_t gameoflife_texture_height;

		int localWorkGroupSize[3];

		unsigned int nthTexture = 0;

		/*	*/
		const std::string computeShaderPath = "Shaders/gameoflife/gameoflife.comp.spv";

		virtual void Release() override {
			glDeleteProgram(this->gameoflife_program);

			glDeleteFramebuffers(1, &this->gameoflife_framebuffer);

			glDeleteTextures(this->gameoflife_texture.size(), (const GLuint *)this->gameoflife_texture.data());
			glDeleteTextures(1, &this->gameoflife_render_texture);
		}

		virtual void Initialize() override {

			/*	*/
			const std::vector<uint32_t> gameoflife_source =
				IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			std::vector<char> gameoflife_source_T =
				fragcore::ShaderCompiler::convertSPIRV(gameoflife_source, compilerOptions);

			/*	Create compute pipeline.	*/
			this->gameoflife_program = ShaderLoader::loadComputeProgram({&gameoflife_source_T});

			/*	Setup compute pipeline.	*/
			glUseProgram(this->gameoflife_program);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "previousCellsTexture"), 0);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "currentCellsTexture"), 1);
			glUniform1i(glGetUniformLocation(this->gameoflife_program, "renderTexture"), 2);
			glGetProgramiv(this->gameoflife_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			glGenFramebuffers(1, &this->gameoflife_framebuffer);
			this->gameoflife_texture.resize(2);
			glGenTextures(this->gameoflife_texture.size(), this->gameoflife_texture.data());

			/*	Create init framebuffers.	*/
			this->onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {
			this->gameoflife_texture_width = width;
			this->gameoflife_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->gameoflife_framebuffer);

			std::vector<uint8_t> textureData(this->gameoflife_texture_width * this->gameoflife_texture_width *
											 sizeof(uint8_t));

			/*	Generate random game state.	*/
			Random random;
			for (size_t j = 0; j < this->gameoflife_texture_height; j++) {
				for (size_t i = 0; i < this->gameoflife_texture_width; i++) {
					/*	Random value between dead and alive cells.	*/
					textureData[this->gameoflife_texture_width * j + i] = Random::range(0, 2);
				}
			}

			/*	Create game of life state textures.	*/
			for (size_t i = 0; i < this->gameoflife_texture.size(); i++) {
				glBindTexture(GL_TEXTURE_2D, this->gameoflife_texture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, this->gameoflife_texture_width, this->gameoflife_texture_height,
							 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, textureData.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Create render target texture to show the result.	*/
			glGenTextures(1, &this->gameoflife_render_texture);
			glBindTexture(GL_TEXTURE_2D, this->gameoflife_render_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->gameoflife_texture_width, this->gameoflife_texture_height, 0,
						 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
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
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{
				glUseProgram(this->gameoflife_program);

				glBindImageTexture(0, this->gameoflife_texture[this->nthTexture % this->gameoflife_texture.size()], 0,
								   GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
				glBindImageTexture(1,
								   this->gameoflife_texture[(this->nthTexture + 1) % this->gameoflife_texture.size()],
								   0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);
				glBindImageTexture(2, this->gameoflife_render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(this->gameoflife_texture_width / (float)this->localWorkGroupSize[0]),
								  std::ceil(this->gameoflife_texture_height / (float)this->localWorkGroupSize[1]), 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			/*	Blit game of life render framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->gameoflife_framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, this->gameoflife_texture_width, this->gameoflife_texture_height, 0, 0, width,
							  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			/*	*/
			this->nthTexture = (this->nthTexture + 1) % this->gameoflife_texture.size();
		}
		virtual void update() override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::GameOfLife> sample(argc, argv);
		sample.run();
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
