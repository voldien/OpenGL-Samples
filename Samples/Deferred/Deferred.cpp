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
			this->fogSettingComponent = std::make_shared<FogSettingComponent>(this->uniform);
			this->addUIComponent(this->fogSettingComponent);

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

		typedef struct material_t {

		} Material;

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

		std::vector<GeometryObject> refObj;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	*/
		unsigned int deferred_framebuffer;

		unsigned int deferred_texture_width;
		unsigned int deferred_texture_height;
		std::vector<unsigned int> deferred_textures; /*	Albedo, WorldSpace, Normal, */
		unsigned int depthTexture;

		unsigned int deferred_program;
		unsigned int multipass_program;

		/*	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		/*	Multipass Shader Source.	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag";

		/*	Deferred Rendering Path.	*/
		const std::string vertexShaderPath = "Shaders/deferred/deferred.vert";
		const std::string fragmentShaderPath = "Shaders/deferred/deferred.frag";

		class FogSettingComponent : public nekomimi::UIComponent {
		  public:
			FogSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Fog Settings");
			}

			void draw() override {
				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Fog Settings");
				ImGui::DragInt("Fog Type", (int *)&this->uniform.fogType);
				ImGui::ColorEdit4("Fog Color", &this->uniform.fogColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Fog Density", &this->uniform.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->uniform.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->uniform.fogStart);
				ImGui::DragFloat("Fog End", &this->uniform.fogEnd);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<FogSettingComponent> fogSettingComponent;

		void Release() override {
			glDeleteProgram(this->deferred_program);
			glDeleteProgram(this->multipass_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = "asset/panoramic.jpg";

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

				/*	Load shader	*/
				this->deferred_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

				/*	Load shader	*/
				this->multipass_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->deferred_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->deferred_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->deferred_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->deferred_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->deferred_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateSphere(1, vertices, indices);
				ProceduralGeometry::generatePlan(1, vertices, indices);

				/*	*/
				ModelImporter modelLoader(FileSystem::getFileSystem());
				modelLoader.loadContent(modelPath, 0);

				const ModelSystemObject &modelRef = modelLoader.getModels()[0];

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->plan.vao);
				glBindVertexArray(this->plan.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->plan.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->plan.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->plan.nrIndicesElements = indices.size();

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
		}

		void onResize(int width, int height) override {

			this->deferred_texture_width = width;
			this->deferred_texture_height = height;

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(deferred_textures.size());
			for (size_t i = 0; i < deferred_textures.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->deferred_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, this->width(), this->height(), 0, GL_RGB, GL_UNSIGNED_BYTE,
							 nullptr);
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->deferred_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			glGenTextures(1, &this->depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->width(), this->height(), 0, GL_DEPTH_COMPONENT,
						 GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

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
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->deferred_framebuffer);
				/*	*/
				glViewport(0, 0, this->deferred_texture_width, this->deferred_texture_height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	*/
				glUseProgram(this->deferred_program);

				glDisable(GL_CULL_FACE);

				/*	Draw triangle.	*/
				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);

				glUseProgram(0);
			}

			// Draw Lights
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glUseProgram(this->deferred_program);
				// Enable blend.

				glDisable(GL_CULL_FACE);
				for (size_t i = 0; i < deferred_textures.size(); i++) {
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, this->deferred_textures[i]);
				}
				/*	*/
				glBindVertexArray(this->sphere.vao);
				glDrawElementsInstanced(GL_TRIANGLES, this->sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										this->pointLights.size());
				glBindVertexArray(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;

			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class DeferredGLSample : public GLSample<Deferred> {
	  public:
		DeferredGLSample() : GLSample<Deferred>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Deferred> sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}