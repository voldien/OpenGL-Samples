#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class MultiPass : public GLSampleWindow {
	  public:
		MultiPass() : GLSampleWindow() {
			this->setTitle("MultiPass");

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

		} uniformBuffer;

		/*	*/
		ModelImporter *modelLoader;

		/*	*/

		enum GBuffer {

			WorldSpace = 1,
			TextureCoordinate = 2,
			Albedo = 0,
			Normal = 3,
			Specular = 2, // Roughness
			Emission = 3,
		};

		Skybox skybox;

		unsigned int multipass_program;
		unsigned int skybox_program;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;

		unsigned int multipass_texture_width;
		unsigned int multipass_texture_height;
		std::vector<unsigned int> multipass_textures;
		unsigned int depthTexture;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		Scene scene;

		/*	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		/*	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			delete this->modelLoader;

			glDeleteProgram(this->multipass_program);
			glDeleteProgram(this->skybox_program);

			/*	*/
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> multipass_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexMultiPassShaderPath, this->getFileSystem());
				const std::vector<uint32_t> multipass_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentMultiPassShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->multipass_program = ShaderLoader::loadGraphicProgram(compilerOptions, &multipass_vertex_binary,
																		   &multipass_fragment_binary);

				/*	Load shader binaries.	*/
				std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->multipass_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniform1i(glGetUniformLocation(this->multipass_program, "AlphaMaskedTexture"), 2);
			glUniformBlockBinding(this->multipass_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
			skybox.Init(skytexture, this->skybox_program);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			modelLoader = new ModelImporter(this->getFileSystem());
			modelLoader->loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(*modelLoader);

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			this->multipass_textures.resize(4);
			glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
			glGenTextures(1, &this->depthTexture);
			this->onResize(this->width(), this->height());
		}

		void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(multipass_textures.size());
			for (size_t i = 0; i < this->multipass_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->multipass_texture_width,
							 this->multipass_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->multipass_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			/*	*/

			glBindTexture(GL_TEXTURE_2D, this->depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture, 0);

			glDrawBuffers(drawAttach.size(), drawAttach.data());

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			/*	*/
			int width, height;
			getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);
				/*	*/
				glViewport(0, 0, this->multipass_texture_width, this->multipass_texture_height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	*/
				glUseProgram(this->multipass_program);

				glDisable(GL_CULL_FACE);

				this->scene.render();

				this->skybox.Render(this->camera);

				glUseProgram(0);
			}

			/*	Blit image targets to screen.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, width, height);

			/*	Transfer each target to default framebuffer.	*/
			const float halfW = (width / 2.0f);
			const float halfH = (height / 2.0f);
			for (size_t i = 0; i < this->multipass_textures.size(); i++) {
				glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
				glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height,
								  (i % 2) * (halfW), (i / 2) * halfH, halfW + (i % 2) * halfW, halfH + (i / 2) * halfH,
								  GL_COLOR_BUFFER_BIT, GL_LINEAR);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(2.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.proj = this->camera.getProjectionMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class MultiPassGLSample : public GLSample<MultiPass> {
	  public:
		MultiPassGLSample() : GLSample<MultiPass>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Texture Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {

		glsample::MultiPassGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}