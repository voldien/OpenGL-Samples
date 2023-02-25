#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Glow : public GLSampleWindow {
	  public:
		Glow() : GLSampleWindow() {
			this->setTitle("Glow");
			this->glowSettingComponent = std::make_shared<GlowSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->glowSettingComponent);
		}

		struct UniformSkyBoxBufferBlock {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
		};

		struct UniformGlowBufferBlock {
			float intensity = 1.0f;
			float threshold = 1.0f;
		};

		struct UniformObjectBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 lookDirection;
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position;

			float IOR = 1.5;
		};

		struct UniformBufferBlock {
			UniformSkyBoxBufferBlock skybox;
			UniformGlowBufferBlock glow;
			UniformObjectBufferBlock ocean;

		} uniformStageBuffer;

		/*	*/
		unsigned int HDRFramebuffer;
		unsigned int shadowTexture;
		size_t hdrWidth;
		size_t hdrHeight;
		unsigned int depthTexture;

		/*	*/
		unsigned int bloomFrameBuffer;
		unsigned int glowTexture;
		size_t bloomWidth;
		size_t bloomHeight;

		/*	*/
		GeometryObject torus;
		GeometryObject plan;
		GeometryObject skybox;

		/*	*/
		unsigned int reflection_texture;

		/*	*/
		unsigned int refrection_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_refrection_buffer_index;
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class GlowSettingComponent : public nekomimi::UIComponent {

		  public:
			GlowSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Bloom Settings");
			}

			virtual void draw() override {
				ImGui::ColorEdit4("Light", &this->uniform.ocean.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ocean.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("IOR", &this->uniform.ocean.IOR);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<GlowSettingComponent> glowSettingComponent;

		CameraController camera;

		const std::string vertexBloomShaderPath = "Shaders/bloom/bloom.vert.spv";
		const std::string fragmentBloomShaderPath = "Shaders/bloom/bloom.frag.spv";

		const std::string vertexRefrectionShaderPath = "Shaders/refrection/refrection.vert.spv";
		const std::string fragmentRefrectionShaderPath = "Shaders/refrection/refrection.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->refrection_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->reflection_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->torus.vao);
			glDeleteBuffers(1, &this->torus.vbo);
			glDeleteBuffers(1, &this->torus.ibo);
		}

		virtual void Initialize() override {

			const std::string panoramicPath = this->getResult()["texture"].as<std::string>();

			/*	Load shader source.	*/
			const std::vector<uint32_t> vertex_refrection_source =
				IOUtil::readFileData<uint32_t>(this->vertexRefrectionShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_refrection_source =
				IOUtil::readFileData<uint32_t>(this->fragmentRefrectionShaderPath, this->getFileSystem());

			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->refrection_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_refrection_source,
																		&fragment_refrection_source);
			this->skybox_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->refrection_program);
			this->uniform_refrection_buffer_index =
				glGetUniformBlockIndex(this->refrection_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->refrection_program, "ReflectionTexture"), 0);
			glUniformBlockBinding(this->refrection_program, this->uniform_refrection_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->reflection_texture = textureImporter.loadImage2D(panoramicPath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateTorus(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->torus.vao);
			glBindVertexArray(this->torus.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->torus.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, torus.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*  Create index buffer.    */
			glGenBuffers(1, &this->torus.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->torus.nrIndicesElements = indices.size();

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

			ProceduralGeometry::generateCube(1, vertices, indices);
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vao);
			glBindVertexArray(this->skybox.vao);

			/*	*/
			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(12));

			glBindVertexArray(0);

			/*  */
			glGenFramebuffers(1, &this->bloomFrameBuffer);
			glGenTextures(1, &this->glowTexture);
			onResize(this->width(), this->height());
		}

		virtual void onResize(int width, int height) override {

			this->hdrWidth = width;
			this->hdrHeight = height;
			this->bloomWidth = width;
			this->bloomHeight = height;
			{
				/*	Create Bloom framebuffer.	*/
				glBindFramebuffer(GL_FRAMEBUFFER, this->HDRFramebuffer);

				/*	*/
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->hdrWidth, this->hdrHeight, 0, GL_RGBA,
							 GL_UNSIGNED_BYTE, nullptr);
				/*	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->shadowTexture, 0);

				/*	*/
				glBindTexture(GL_TEXTURE_2D, depthTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->hdrWidth, this->hdrHeight, 0,
							 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
				glBindTexture(GL_TEXTURE_2D, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthTexture, 0);

				unsigned int deo = GL_COLOR_ATTACHMENT0;
				glDrawBuffers(1, &deo);

				int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &HDRFramebuffer);
					// TODO add error message.
					throw RuntimeException("Failed to create framebuffer, {}",
										   (const char *)glewGetErrorString(frstat));
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				/*	Create Bloom framebuffer.	*/
				glBindFramebuffer(GL_FRAMEBUFFER, this->bloomFrameBuffer);

				/*	*/
				glGenTextures(1, &this->glowTexture);
				glBindTexture(GL_TEXTURE_2D, this->glowTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->bloomWidth, this->bloomHeight, 0, GL_RGBA,
							 GL_UNSIGNED_BYTE, nullptr);
				/*	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5));

				FVALIDATE_GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->glowTexture, 0);

				deo = GL_COLOR_ATTACHMENT0;
				glDrawBuffers(1, &deo);

				frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &HDRFramebuffer);
					// TODO add error message.
					throw RuntimeException("Failed to create framebuffer, {}",
										   (const char *)glewGetErrorString(frstat));
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			this->uniformStageBuffer.ocean.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->HDRFramebuffer);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*  Refrection. */
			{
				glUseProgram(this->refrection_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->glowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->torus.vao);
				glDrawElements(GL_TRIANGLES, this->torus.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Skybox. */
			{
				glUseProgram(this->skybox_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				// glDepthMask(GL_FALSE);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->glowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->torus.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Blit glow framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->HDRFramebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			/*	Downscale.	*/
			glBindTexture(GL_TEXTURE_2D, this->glowTexture);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, this->bloomWidth, this->bloomHeight);
			glGenerateMipmap(GL_TEXTURE_2D);

			/*	Bloom.	*/
		}

		virtual void update() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformStageBuffer.ocean.model = glm::mat4(1.0f);
			this->uniformStageBuffer.ocean.model =
				glm::rotate(this->uniformStageBuffer.ocean.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniformStageBuffer.ocean.model = glm::scale(this->uniformStageBuffer.ocean.model, glm::vec3(10.95f));
			this->uniformStageBuffer.ocean.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.ocean.lookDirection = glm::vec4(this->camera.getLookDirection(), 0);

			this->uniformStageBuffer.ocean.modelViewProjection =
				this->uniformStageBuffer.ocean.proj * this->uniformStageBuffer.ocean.view * this->uniformStageBuffer.ocean.model;
			this->uniformStageBuffer.ocean.position = glm::vec4(this->camera.getPosition(), 0);

			/*  */
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class GlowGLSample : public GLSample<Glow> {
	  public:
		GlowGLSample() : GLSample<Glow>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/panoramic.jpg"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::GlowGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}