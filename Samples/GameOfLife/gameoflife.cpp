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

		struct UniformBufferBlock {
			float posX, posY;
			float mousePosX, mousePosY;
			float zoom; /*  */
			float c;	/*  */
			int nrSamples;
		} params;

		unsigned int gameoflife_framebuffer;
		unsigned int gameoflife_program;
		unsigned int gameoflife_texture;
		unsigned int gameoflife_texture_width;
		unsigned int gameoflife_texture_height;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		/*	*/
		const std::string computeShaderPath = "Shaders/gameoflife/gameoflife.comp";

		virtual void Release() override {
			glDeleteProgram(this->gameoflife_program);
			glDeleteFramebuffers(1, &this->gameoflife_framebuffer);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, (const GLuint *)&this->gameoflife_texture);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> gameoflife_source = IOUtil::readFileString(computeShaderPath, this->getFileSystem());

			// gameoflife_source =
			// 	fragcore::ShaderCompiler::convertSPIRV(gameoflife_source, fragcore::ShaderLanguage::GLSL);

			/*	Load shader	*/
			this->gameoflife_program = ShaderLoader::loadComputeProgram({&gameoflife_source});

			/*	*/
			glUseProgram(this->gameoflife_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->gameoflife_program, "UniformBufferBlock");
			glUniformBlockBinding(this->gameoflife_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1iARB(glGetUniformLocation(this->gameoflife_program, "img_output"), 0);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align<size_t>(uniformBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenFramebuffers(1, &this->gameoflife_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->gameoflife_framebuffer);

			glGenTextures(1, &this->gameoflife_texture);
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {
			this->gameoflife_texture_width = width;
			this->gameoflife_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->gameoflife_framebuffer);
			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->gameoflife_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gameoflife_texture, 0);

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

			this->update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			{
				glUseProgram(this->gameoflife_program);
				const size_t localInvokation = 32;

				glBindImageTexture(0, this->gameoflife_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
			/*	Blit mandelbrot framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->gameoflife_framebuffer);
			glBlitFramebuffer(0, 0, gameoflife_texture_width, gameoflife_texture_height, 0, 0, width, height,
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		virtual void update() {

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount()) % nrUniformBuffer) * uniformBufferSize,
								 uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(p, &params, sizeof(params));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);

			/*	Update.	*/
			int x, y;
			SDL_GetMouseState(&x, &y);
			params.mousePosX = x;
			params.mousePosY = y;
			params.posX = 0;
			params.posY = 0;
			params.zoom = 1.0f;
			params.nrSamples = 128;
		}
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