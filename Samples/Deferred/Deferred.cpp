#include "Scene.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * Deferred Rendering Path Sample.
	 **/
	class Deferred : public GLSampleWindow {
	  public:
		Deferred() : GLSampleWindow() {
			this->setTitle("Deferred Rendering");

			/*	Setting Window.	*/
			this->deferredSettingComponent = std::make_shared<DeferredSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->deferredSettingComponent);

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

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} uniformBuffer;

		typedef struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;
		} PointLight;

		std::vector<PointLight> pointLights;

		/*	*/
		GeometryObject plan;   /*	Directional light.	*/
		GeometryObject sphere; /*	Point Light.*/
		Scene scene;		   /*	World Scene.	*/

		/*	*/
		unsigned int deferred_framebuffer;
		unsigned int deferred_texture_width;
		unsigned int deferred_texture_height;
		std::vector<unsigned int> deferred_textures; /*	Albedo, WorldSpace, Normal, */
		unsigned int depthTexture;

		/*	*/
		unsigned int deferred_program;
		unsigned int multipass_program;
		unsigned int skybox_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_pointlight_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_pointlight_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t uniformLightBufferSize = sizeof(UniformBufferBlock);

		/*	*/
		CameraController camera;

		/*	Multipass Shader Source.	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		/*	Deferred Rendering Shader Path.	*/
		const std::string vertexShaderPath = "Shaders/deferred/deferred.vert.spv";
		const std::string fragmentShaderPath = "Shaders/deferred/deferred.frag.spv";

		/*	Skybox Shader Path.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		class DeferredSettingComponent : public nekomimi::UIComponent {
		  public:
			DeferredSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Deferred Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Direction Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				int tmp;
				if (ImGui::DragInt("Number of Lights", &tmp)) {
					// TODO add
				}

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<DeferredSettingComponent> deferredSettingComponent;

		void Release() override {
			glDeleteProgram(this->deferred_program);
			glDeleteProgram(this->multipass_program);
			glDeleteProgram(this->skybox_program);

			/*	*/
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->deferred_textures.size(), this->deferred_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexMultiPassShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentMultiPassShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());

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

				/*	Load shader	*/
				this->deferred_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

				/*	Load shader	*/
				this->multipass_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->multipass_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->multipass_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->deferred_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->deferred_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->deferred_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->deferred_program, "NormalTexture"), 1);
			glUniform1i(glGetUniformLocation(this->deferred_program, "WorldTexture"), 2);
			glUniformBlockBinding(this->deferred_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			// TODO setup skybox.

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*	Load Light geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateSphere(1, vertices, indices);
				// Compute offset.
				ProceduralGeometry::generatePlan(1, vertices, indices);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->plan.vao);
				glBindVertexArray(this->plan.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->plan.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex),
							 vertices.data(), GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->plan.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->plan.nrIndicesElements = indices.size();
				this->sphere.nrIndicesElements = 0;

				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				/*	UV.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				/*	Normal.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}

			{
				/*	Create deferred framebuffer.	*/
				glGenFramebuffers(1, &this->deferred_framebuffer);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->deferred_framebuffer);

				this->deferred_textures.resize(4);
				glGenTextures(this->deferred_textures.size(), this->deferred_textures.data());
				this->onResize(this->width(), this->height());
			}

			/*	Setup lights.	*/
			this->pointLights.resize(64);
			for (size_t i = 0; i < this->pointLights.size(); i++) {
				this->pointLights[i].range = 25.0f;
				this->pointLights[i].position = glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(2.0f);
				this->pointLights[i].color = glm::vec4(1, 1, 1, 1);
				this->pointLights[i].constant_attenuation = 1.7f;
				this->pointLights[i].linear_attenuation = 1.5f;
				this->pointLights[i].qudratic_attenuation = 0.19f;
				this->pointLights[i].intensity = 1.0f;
			}
		}

		void onResize(int width, int height) override {

			this->deferred_texture_width = width;
			this->deferred_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->deferred_framebuffer);

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(deferred_textures.size());
			for (size_t i = 0; i < this->deferred_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->deferred_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->deferred_texture_width, this->deferred_texture_height,
							 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->deferred_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			/*	*/
			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->deferred_texture_width,
						 this->deferred_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
			this->getSize(&width, &height);

			/*	*/

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	Multipass */
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->deferred_framebuffer);
				/*	*/
				glViewport(0, 0, this->deferred_texture_width, this->deferred_texture_height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	*/
				glUseProgram(this->multipass_program);

				glDepthMask(GL_TRUE);
				glDisable(GL_CULL_FACE);

				this->scene.render();

				glUseProgram(0);
			}

			/* Draw Lights.	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_pointlight_buffer_binding, uniform_pointlight_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->deferred_program);

				// Enable blend.
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_ONE, GL_ONE);

				glDepthMask(GL_FALSE);
				glDisable(GL_DEPTH_TEST);

				glDisable(GL_CULL_FACE);
				for (size_t i = 0; i < this->deferred_textures.size(); i++) {
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, this->deferred_textures[i]);
				}

				/*	*/
				glBindVertexArray(this->plan.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										this->pointLights.size());
				glBindVertexArray(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			{
				this->uniformBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformBuffer.model = glm::mat4(1.0f);
				this->uniformBuffer.model = glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f),
														glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(1.95f));
				this->uniformBuffer.view = this->camera.getViewMatrix();
				this->uniformBuffer.modelViewProjection =
					this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			}

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

	class DeferredGLSample : public GLSample<Deferred> {
	  public:
		DeferredGLSample() : GLSample<Deferred>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Texture Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::DeferredGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}