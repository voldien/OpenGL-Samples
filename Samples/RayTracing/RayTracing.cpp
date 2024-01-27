#include "ModelImporter.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <Scene.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <Skybox.h>
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
		RayTracing() : GLSampleWindow() {
			this->setTitle("RayTracing - Compute");

			/*	Setting Window.	*/
			this->raytracingSettingComponent = std::make_shared<RayTracingSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->raytracingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			unsigned int sampleIndex;

		} uniformBuffer;
		/*	Framebuffers.	*/
		unsigned int raytracing_framebuffer;
		unsigned int raytracing_render_texture; /*	No round robin required, since once updated, it is instantly
												   blitted. thus no implicit sync between frames.	*/
		unsigned int raytracing_display_texture;
		size_t raytracing_texture_width;
		size_t raytracing_texture_height;

		Scene scene;   /*	World Scene.	*/
		Skybox skybox; /*	*/
		CameraController camera;

		/*	*/
		unsigned int raytracing_program;
		int localWorkGroupSize[3];

		unsigned int nthTexture = 0;

		unsigned int skytexture;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_pointlight_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_pointlight_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t uniformLightBufferSize = 0;

		/*	*/
		const std::string computeShaderPath = "Shaders/raytracing/raytracing.comp.spv";

		class RayTracingSettingComponent : public nekomimi::UIComponent {
		  public:
			RayTracingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("RayTracing Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Direction Light Settings");
				// ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				// ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				// ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				int tmp;
				if (ImGui::DragInt("Max Samples", &MaxSamples)) {
					// TODO add
				}

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("DrawLight", &this->showLight);
			}

			bool showWireFrame = false;
			bool showLight = false;
			int MaxSamples;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<RayTracingSettingComponent> raytracingSettingComponent;

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
			int uniform_buffer_index = glGetUniformBlockIndex(this->raytracing_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->raytracing_program, "renderTexture"), 0);
			glGetProgramiv(this->raytracing_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUniformBlockBinding(this->raytracing_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
			// this->uniformLightBufferSize = (sizeof(pointLights[0]) * pointLights.size());
			// this->uniformLightBufferSize = fragcore::Math::align(uniformLightBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create sampler distrubution.	*/

			/*	*/
			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skytexture = textureImporter.loadImage2D(panoramicPath);

			/*	Load scene from model importer.	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

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

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	Bind and Compute Game of Life Compute Program.	*/
			{
				glUseProgram(this->raytracing_program);

				/*	The image where the graphic version will be stored as.	*/
				glBindImageTexture(0, this->raytracing_render_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
				glBindImageTexture(1, this->skytexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

				// glBindBufferBase(
				//	GL_SHADER_STORAGE_BUFFER, this->previous_cells_buffer_binding,
				//	this->reactiondiffusion_buffer[(this->nthTexture + 1) % this->reactiondiffusion_buffer.size()]);

				glDispatchCompute(std::ceil(this->raytracing_texture_width / (float)this->localWorkGroupSize[0]),
								  std::ceil(this->raytracing_texture_height / (float)this->localWorkGroupSize[1]), 1);

				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{

				this->uniformBuffer.model = glm::mat4(1.0f);
				this->uniformBuffer.model = glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f),
														glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(1.95f));
				this->uniformBuffer.view = this->camera.getViewMatrix();
				this->uniformBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformBuffer.modelViewProjection =
					this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			}

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class RayTracingGLSample : public GLSample<RayTracing> {
	  public:
		RayTracingGLSample() : GLSample<RayTracing>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Skybox Texture File Path",
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
