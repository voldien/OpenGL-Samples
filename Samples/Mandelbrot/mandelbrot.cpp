#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <iostream>
namespace glsample {

	class MandelBrot : public GLSampleWindow {
	  public:
		MandelBrot() : GLSampleWindow() { this->setTitle("Texture"); }

		struct UniformBufferBlock {
			float posX, posY;
			float mousePosX, mousePosY;
			float zoom; /*  */
			float c;	/*  */
			int nrSamples;
		} params;

		unsigned int mandelbrot_framebuffer;
		unsigned int mandelbrot_program;
		unsigned int gl_texture;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		/*	*/
		const std::string computeShaderPath = "Shaders/mandelbrot/mandelbrot.comp.spv";

		virtual void Release() override {
			glDeleteProgram(this->mandelbrot_program);

			glDeleteTextures(1, (const GLuint *)&this->gl_texture);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> mandelbrot_source = IOUtil::readFileData(computeShaderPath);

			mandelbrot_source =
				fragcore::ShaderCompiler::convertSPIRV(mandelbrot_source, fragcore::ShaderLanguage::GLSL);

			/*	Load shader	*/
			this->mandelbrot_program = ShaderLoader::loadComputeProgram({&mandelbrot_source});

			glUseProgram(this->mandelbrot_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->mandelbrot_program, "params");
			glUniformBlockBinding(this->mandelbrot_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1iARB(glGetUniformLocation(this->mandelbrot_program, "img_output"), 0);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &minMapBufferSize);
			uniformSize += uniformSize % minMapBufferSize;

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenFramebuffers(1, &this->mandelbrot_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);

			glGenTextures(1, &this->gl_texture);
			onResize(this->width(), this->height());

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gl_texture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void onResize(int width, int height) override {
			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);
			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->gl_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->gl_texture, 0);
		}

		virtual void draw() override {

			this->update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glUseProgram(this->mandelbrot_program);
			const int localInvokation = 32;

			glBindImageTexture(0, this->gl_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

			glDispatchCompute(std::ceil(width / localInvokation), std::ceil(height / localInvokation), 1);

			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->mandelbrot_framebuffer);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		virtual void update() {

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT);
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
		GLSample<glsample::MandelBrot> sample(argc, argv);
		sample.run();
	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
