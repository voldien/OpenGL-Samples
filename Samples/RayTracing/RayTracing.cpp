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
	class RayTracing : public GLSampleWindow {
	  public:
		RayTracing() : GLSampleWindow() { this->setTitle("RayTracing - Compute"); }

		/*	Framebuffers.	*/
		unsigned int raytracing_framebuffer;
		unsigned int raytracing_render_texture; /*	No round robin required, since once updated, it is instantly
												   blitted. thus no implicit sync between frames.	*/
		size_t raytracing_texture_width;
		size_t raytracing_texture_height;

		CameraController camera;
		/*	*/
		unsigned int raytracing_program;
		int localWorkGroupSize[3];

		unsigned int nthTexture = 0;

		/*	*/
		const std::string computeShaderPath = "Shaders/raytracing/raytracing.comp.spv";

		void Release() override {
			glDeleteProgram(this->raytracing_program);

			glDeleteFramebuffers(1, &this->raytracing_framebuffer);

			glDeleteTextures(1, &this->raytracing_render_texture);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> raytracing_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->raytracing_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &raytracing_compute_binary);
			}

			/*	Setup compute pipeline.	*/
			glUseProgram(this->raytracing_program);
			glUniform1i(glGetUniformLocation(this->raytracing_program, "renderTexture"), 0);
			glGetProgramiv(this->raytracing_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			// this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
			// this->uniformLightBufferSize = (sizeof(pointLights[0]) * pointLights.size());
			// this->uniformLightBufferSize = fragcore::Math::align(uniformLightBufferSize, (size_t)minMapBufferSize);

			{
				/*	Create framebuffer and its textures.	*/
				glGenFramebuffers(1, &this->raytracing_framebuffer);
				// this->gameoflife_state_texture.resize(2);
				glGenTextures(1, &this->raytracing_render_texture);
				/*	Create init framebuffers.	*/
				this->onResize(this->width(), this->height());
			}
		}

		void onResize(int width, int height) override {

			this->raytracing_texture_width = width;
			this->raytracing_texture_height = height;

			glBindFramebuffer(GL_FRAMEBUFFER, this->raytracing_framebuffer);

			/*	Create render target texture to show the result.	*/
			glBindTexture(GL_TEXTURE_2D, this->raytracing_render_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->raytracing_texture_width, this->raytracing_texture_height, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glBindTexture(GL_TEXTURE_2D, 0);

			/*	*/
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->raytracing_render_texture,
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

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	Bind and Compute Game of Life Compute Program.	*/
			{
				glUseProgram(this->raytracing_program);

				/*	The image where the graphic version will be stored as.	*/
				glBindImageTexture(0, this->raytracing_render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

				glDispatchCompute(std::ceil(this->raytracing_texture_width / (float)this->localWorkGroupSize[0]),
								  std::ceil(this->raytracing_texture_height / (float)this->localWorkGroupSize[1]), 1);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}

			/*	*/
			glViewport(0, 0, width, height);

			/*	Blit game of life render framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->raytracing_framebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			/*	Blit with nearset to retain the details of each of the cells states.	*/
			glBlitFramebuffer(0, 0, this->raytracing_texture_width, this->raytracing_texture_height, 0, 0, width,
							  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

			/*	Update the next frame in round robin.	*/
			// this->nthTexture = (this->nthTexture + 1) % this->gameoflife_state_texture.size();
		}
		void update() override {}
	};

	class RayTracingGLSample : public GLSample<RayTracing> {
	  public:
		RayTracingGLSample() : GLSample<RayTracing>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Texture Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::RayTracingGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
