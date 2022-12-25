#include "GLSampleWindow.h"

#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <ShaderCompiler.h>

#include <glm/glm.hpp>
#include <iostream>
namespace glsample {

	class MandelBrot : public GLSampleWindow {
	  public:
		MandelBrot() : GLSampleWindow() {
			this->setTitle("MandelBrot-Compute");
			mandelbrotSettingComponent = std::make_shared<MandelBrotSettingComponent>(this->params);
			this->addUIComponent(mandelbrotSettingComponent);
		}

		struct UniformBufferBlock {
			float posX, posY;
			float mousePosX, mousePosY;
			float zoom; /*  */
			float c;	/*  */
			int nrSamples = 128;
		} params;

		/*	*/
		unsigned int mandelbrot_framebuffer;
		unsigned int mandelbrot_program;
		unsigned int mandelbrot_texture;
		unsigned int mandelbrot_texture_width;
		unsigned int mandelbrot_texture_height;

		int localWorkGroupSize[3];

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class MandelBrotSettingComponent : public nekomimi::UIComponent {

		  public:
			MandelBrotSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Mandelbrot Settings");
			}
			virtual void draw() override {
				ImGui::DragInt("Number of Samples", &this->uniform.nrSamples, 1, 0, 128);
				ImGui::DragFloat("C", &this->uniform.c);
				//				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);
				//				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				//				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<MandelBrotSettingComponent> mandelbrotSettingComponent;

		/*	*/
		const std::string computeShaderPath = "Shaders/mandelbrot/mandelbrot.comp";

		virtual void Release() override {

			glDeleteProgram(this->mandelbrot_program);
			glDeleteFramebuffers(1, &this->mandelbrot_framebuffer);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, (const GLuint *)&this->mandelbrot_texture);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> mandelbrot_source = IOUtil::readFileString(computeShaderPath, this->getFileSystem());

			// mandelbrot_source =
			// 	fragcore::ShaderCompiler::convertSPIRV(mandelbrot_source, fragcore::ShaderLanguage::GLSL);

			/*	Load shader	*/
			this->mandelbrot_program = ShaderLoader::loadComputeProgram({&mandelbrot_source});

			glUseProgram(this->mandelbrot_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->mandelbrot_program, "UniformBufferBlock");
			glUniformBlockBinding(this->mandelbrot_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1iARB(glGetUniformLocation(this->mandelbrot_program, "img_output"), 0);
			glGetProgramiv(this->mandelbrot_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align<size_t>(uniformBufferSize, minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenFramebuffers(1, &this->mandelbrot_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);

			glGenTextures(1, &this->mandelbrot_texture);
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

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
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			{

				glUseProgram(this->mandelbrot_program);
				glBindImageTexture(0, this->mandelbrot_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				int localWorkGroupSize[3];

				glGetProgramiv(this->mandelbrot_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

				glDispatchCompute(std::ceil(this->mandelbrot_texture_width / (float)localWorkGroupSize[0]),
								  std::ceil(this->mandelbrot_texture_height / (float)localWorkGroupSize[1]), 1);

				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			}

			/*	Blit mandelbrot framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->mandelbrot_framebuffer);

			glBlitFramebuffer(0, 0, this->mandelbrot_texture_width, this->mandelbrot_texture_height, 0, 0, width,
							  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::MandelBrot> sample(argc, argv);
		sample.run();
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
