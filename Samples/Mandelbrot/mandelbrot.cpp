#include "Input.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class MandelBrot : public GLSampleWindow {
	  public:
		MandelBrot() : GLSampleWindow() {
			this->setTitle("MandelBrot Compute");

			/*	*/
			this->mandelbrotSettingComponent = std::make_shared<MandelBrotSettingComponent>(this->params);
			this->addUIComponent(this->mandelbrotSettingComponent);
		}

		struct uniform_buffer_block {
			float posX, posY;
			float mousePosX, mousePosY;
			float zoom = 1.0f; /*	*/
			float c = 0;	   /*	*/
			float ci = 1;	   /*	*/
			int nrSamples = 128;
		} params;

		/*	*/
		const size_t round_robin_size = 2;
		unsigned int mandelbrot_framebuffer;
		unsigned int mandelbrot_program;
		unsigned int julia_program;
		unsigned int mandelbrot_texture; // TODO add round robin.
		unsigned int mandelbrot_texture_width;
		unsigned int mandelbrot_texture_height;

		/*	*/
		int localWorkGroupSize[3];

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		class MandelBrotSettingComponent : public nekomimi::UIComponent {

		  public:
			MandelBrotSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Mandelbrot Settings");
			}
			void draw() override {
				ImGui::DragInt("Number of Samples", &this->uniform.nrSamples, 1, 0, 2048);
				ImGui::DragFloat2("C", &this->uniform.c);
				ImGui::DragFloat("Zoom", &this->uniform.zoom, 1.0f, 0.001, 10.0f);
				ImGui::DragInt("Program", &this->program, 1.0f, 0, 1);
			}

			bool showWireFrame = false;
			int program;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<MandelBrotSettingComponent> mandelbrotSettingComponent;

		/*	*/
		const std::string computeMandelbrotShaderPath = "Shaders/mandelbrot/mandelbrot.comp.spv";
		const std::string computeJuliaShaderPath = "Shaders/mandelbrot/julia.comp.spv";

		void Release() override {

			glDeleteProgram(this->mandelbrot_program);
			glDeleteProgram(this->julia_program);
			glDeleteFramebuffers(1, &this->mandelbrot_framebuffer);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, (const GLuint *)&this->mandelbrot_texture);
		}

		void Initialize() override {

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> mandelbrot_binary =
					IOUtil::readFileData<uint32_t>(this->computeMandelbrotShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->mandelbrot_program = ShaderLoader::loadComputeProgram(compilerOptions, &mandelbrot_binary);

				const std::vector<uint32_t> julia_binary =
					IOUtil::readFileData<uint32_t>(this->computeJuliaShaderPath, this->getFileSystem());

				/*	Load shader	*/
				this->julia_program = ShaderLoader::loadComputeProgram(compilerOptions, &julia_binary);
			}

			/*	*/
			glUseProgram(this->mandelbrot_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->mandelbrot_program, "UniformBufferBlock");
			glUniformBlockBinding(this->mandelbrot_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1i(glGetUniformLocation(this->mandelbrot_program, "img_output"), 0);
			glGetProgramiv(this->mandelbrot_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->julia_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->julia_program, "UniformBufferBlock");
			glUniformBlockBinding(this->julia_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1i(glGetUniformLocation(this->julia_program, "img_output"), 0);
			glGetProgramiv(this->julia_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align<size_t>(this->uniformBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			{
				glGenFramebuffers(1, &this->mandelbrot_framebuffer);

				glGenTextures(1, &this->mandelbrot_texture);
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->mandelbrot_texture_width = width;
			this->mandelbrot_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);
			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->mandelbrot_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->mandelbrot_texture, 0);

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

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{

				if (this->mandelbrotSettingComponent->program == 0) {
					glUseProgram(this->mandelbrot_program);
				} else {
					glUseProgram(this->julia_program);
				}

				glBindImageTexture(0, this->mandelbrot_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(this->mandelbrot_texture_width / (float)localWorkGroupSize[0]),
								  std::ceil(this->mandelbrot_texture_height / (float)localWorkGroupSize[1]), 1);

				glUseProgram(0);
				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			}

			/*	Blit mandelbrot framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->mandelbrot_framebuffer);

			glBlitFramebuffer(0, 0, this->mandelbrot_texture_width, this->mandelbrot_texture_height, 0, 0, width,
							  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void update() override {

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount()) % nrUniformBuffer) * uniformBufferSize,
								 uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &params, sizeof(params));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update Position.	*/
			{
				static int prev_move_X = 0, prev_move_Y = 0;
				static int prev_zoom_X = 0, prev_zoom_zoom = 0;

				if (this->getInput().getMouseDown(Input::MouseButton::LEFT_BUTTON)) {
					this->getInput().getMousePosition(&prev_move_X, &prev_move_Y);
				}

				if (this->getInput().getMouseReleased(Input::MouseButton::LEFT_BUTTON)) {
					params.posX = params.mousePosX;
					params.posY = params.mousePosY;
				}

				if (this->getInput().getMouseDown(Input::MouseButton::RIGHT_BUTTON)) {
					this->getInput().getMousePosition(&prev_zoom_X, nullptr);
					prev_zoom_zoom = params.zoom;
				}

				if (this->getInput().getMouseReleased(Input::MouseButton::RIGHT_BUTTON)) {
				}

				int x, y;
				if (this->getInput().getMousePosition(&x, &y)) {
					if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON)) {
						const int deltaX = -(x - prev_move_X);
						const int deltaY = (y - prev_move_Y);
						params.mousePosX = params.posX + deltaX;
						params.mousePosY = params.posY + deltaY;
					}
					if (this->getInput().getMousePressed(Input::MouseButton::RIGHT_BUTTON)) {
						const int deltaZoomX = -(x - prev_zoom_X);

						params.zoom = prev_zoom_zoom + deltaZoomX * 0.0001f;
					}
				}
			}
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::MandelBrot> sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
