#include "Exception.hpp"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

namespace glsample {

	class MultiSamplingAntiAliasing {};
	
	/**
	 * @brief 
	 * 
	 */
	class AntiAliasing : public GLSampleWindow {
	  public:
		AntiAliasing() : GLSampleWindow() {
			this->setTitle("AntiAliasing");

			/*	Setting Window.	*/
			this->antialiasingSettingComponent =
				std::make_shared<AntiAliasingSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->antialiasingSettingComponent);


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
		std::vector<GeometryObject> refObj;
		ModelImporter *modelLoader;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer;
		unsigned int multipass_program;
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

		/*	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		class AntiAliasingSettingComponent : public nekomimi::UIComponent {
		  public:
			AntiAliasingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Alpha Clipping Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Alpha Clipping Setting");
				ImGui::DragFloat("Clipping", &this->uniform.clipping, 0.035f, 0.0f, 1.0f);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<AntiAliasingSettingComponent> antialiasingSettingComponent;

		void Release() override {
			delete this->modelLoader;

			glDeleteProgram(this->multipass_program);
			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			for (size_t i = 0; i < this->refObj.size(); i++) {
				if (glIsVertexArray(this->refObj[i].vao)) {
					glDeleteVertexArrays(1, &this->refObj[i].vao);
					glDeleteBuffers(1, &this->refObj[i].vbo);
					glDeleteBuffers(1, &this->refObj[i].ibo);
				}
			}
		}

		void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();
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
			}
			/*	Setup graphic pipeline.	*/
			glUseProgram(this->multipass_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->multipass_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/ // TODO remove.
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);

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

			/*	*/
			ImportHelper::loadModelBuffer(*modelLoader, refObj);
			ImportHelper::loadTextures(*modelLoader);

			/*	Create multipass framebuffer.	*/
			glGenFramebuffers(1, &this->multipass_framebuffer);

			/*	*/
			this->multipass_textures.resize(4);
			glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
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
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->multipass_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			/*	*/
			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
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
		}

		void draw() override {

			/*	*/
			int width, height;
			getSize(&width, &height);

			/*	*/
			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

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

				glBindVertexArray(this->refObj[0].vao);
				for (size_t x = 0; x < this->modelLoader->getNodes().size(); x++) {

					/*	*/
					NodeObject *node = this->modelLoader->getNodes()[x];

					for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, this->modelLoader->getTextures()[0].texture);

						glDrawElementsBaseVertex(
							GL_TRIANGLES, this->refObj[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
							(void *)(sizeof(unsigned int) * this->refObj[node->geometryObjectIndex[i]].indices_offset),
							this->refObj[node->geometryObjectIndex[i]].vertex_offset);
					}
				}
				glBindVertexArray(0);

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
			const float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(40.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
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
	class AntiAliasingGLSample : public GLSample<AntiAliasing> {
	  public:
		AntiAliasingGLSample() : GLSample<AntiAliasing>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::AntiAliasingGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}