#include "Input.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ModelViewer.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class OpticalFlow : public ModelViewer {
	  public:
		OpticalFlow() : ModelViewer() {
			this->setTitle("OpticalFlow");

			/*	*/
			this->opticalflowSettingComponent = std::make_shared<OpticalFlowSettingComponent>(this->params);
			this->addUIComponent(this->opticalflowSettingComponent);
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
		unsigned int color_framebuffer;
		unsigned int opticalTexture;

		unsigned int opticalflow_program;
		unsigned int color_texture; // TODO add round robin.
		unsigned int color_texture_width;
		unsigned int color_texture_height;

		/*	*/
		int localWorkGroupSize[3];

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		class OpticalFlowSettingComponent : public nekomimi::UIComponent {

		  public:
			OpticalFlowSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("OpticalFlow Settings");
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
		std::shared_ptr<OpticalFlowSettingComponent> opticalflowSettingComponent;

		/*	*/
		const std::string computeMandelbrotShaderPath = "Shaders/opticalflow/opticalflow.comp.spv";

		void Release() override {
			ModelViewer::Release();

			glDeleteProgram(this->opticalflow_program);
			glDeleteFramebuffers(1, &this->color_framebuffer);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteTextures(1, (const GLuint *)&this->color_texture);
		}

		void Initialize() override {
			ModelViewer::Initialize();
			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> optical_flow_source =
					IOUtil::readFileData<uint32_t>(this->computeMandelbrotShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				const std::vector<char> optical_flow_binary =
					fragcore::ShaderCompiler::convertSPIRV(optical_flow_source, compilerOptions);

				/*	Load shader	*/
				this->opticalflow_program = ShaderLoader::loadComputeProgram({&optical_flow_binary});
			}

			/*	*/
			glUseProgram(this->opticalflow_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->opticalflow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->opticalflow_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniform1i(glGetUniformLocation(this->opticalflow_program, "img_output"), 0);
			glGetProgramiv(this->opticalflow_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align<size_t>(this->uniformBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			{
				glGenFramebuffers(1, &this->color_framebuffer);

				glGenTextures(1, &this->color_texture);
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {
			ModelViewer::onResize(width, height);

			this->color_texture_width = width;
			this->color_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->color_framebuffer);
			/*	Resize the image.	*/
			glBindTexture(GL_TEXTURE_2D, this->color_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->color_texture, 0);

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

			ModelViewer::draw();

			int width, height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->color_framebuffer);
			this->getSize(&width, &height);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			{
				glUseProgram(this->opticalflow_program);

				glBindImageTexture(0, this->opticalTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
				glBindImageTexture(1, this->color_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);


				glDispatchCompute(std::ceil(this->color_texture_width / (float)localWorkGroupSize[0]),
								  std::ceil(this->color_texture_height / (float)localWorkGroupSize[1]), 1);

				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			}

			/*	Blit mandelbrot framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->color_framebuffer);

			glBlitFramebuffer(0, 0, this->color_texture_width, this->color_texture_height, 0, 0, width, height,
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	Blit image targets to screen.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->color_framebuffer);

			glViewport(0, 0, width, height);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, this->color_texture_width, this->color_texture_height, 0, 0,
							  this->color_texture_width, this->color_texture_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			/*	Transfer each target to default framebuffer.	*/
			const size_t widthDivior = 2;
			const size_t heightDivior = 2;

			// const float halfW = (width / 2.0f);
			// const float halfH = (height / 2.0f);
			// for (size_t i = 0; i < this->multipass_textures.size(); i++) {
			//	glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
			//	glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height,
			//					  (i % 2) * (halfW), (i / 2) * halfH, halfW + (i % 2) * halfW, halfH + (i / 2) * halfH,
			//					  GL_COLOR_BUFFER_BIT, GL_LINEAR);
			//}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void update() override {
			ModelViewer::update();
			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount()) % nrUniformBuffer) * uniformBufferSize,
								 uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &params, sizeof(params));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update Position.	*/
			{
				static int prevX = 0, prevY = 0;
				if (this->getInput().getMouseDown(Input::MouseButton::LEFT_BUTTON)) {
					this->getInput().getMousePosition(&prevX, &prevY);
				}

				if (this->getInput().getMouseReleased(Input::MouseButton::LEFT_BUTTON)) {
					params.posX = params.mousePosX;
					params.posY = params.mousePosY;
				}

				int x, y;
				if (this->getInput().getMousePosition(&x, &y)) {
					const int deltaX = -(x - prevX);
					const int deltaY = (y - prevY);
					params.mousePosX = params.posX + deltaX;
					params.mousePosY = params.posY + deltaY;
				}
			}
		}
	};

	class OpticalFlowGLSample : public GLSample<OpticalFlow> {
	  public:
		OpticalFlowGLSample() : GLSample<OpticalFlow>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::OpticalFlowGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
