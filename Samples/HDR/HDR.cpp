#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class HDR : public GLSampleWindow {
	  public:
		HDR() : GLSampleWindow() {
			this->setTitle("HDR");
			this->refrectionSettingComponent = std::make_shared<RefrectionSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->refrectionSettingComponent);
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 lookDirection;
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position;

			float IOR = 1.5;

		} uniformBuffer;

		/*	*/
		unsigned int HDRFramebuffer;
		unsigned int shadowTexture;
		size_t shadowWidth;
		size_t shadowHeight;
		unsigned int depthTexture;

		/*	*/
		GeometryObject torus;
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

		class RefrectionSettingComponent : public nekomimi::UIComponent {

		  public:
			RefrectionSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Refrection Settings");
			}
			virtual void draw() override {
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("IOR", &this->uniform.IOR);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<RefrectionSettingComponent> refrectionSettingComponent;

		CameraController camera;

		std::string panoramicPath = "asset/panoramic.jpg";

		const std::string vertexRefrectionShaderPath = "Shaders/refrection/refrection.vert";
		const std::string fragmentRefrectionShaderPath = "Shaders/refrection/refrection.frag";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag";

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

			/*	Load shader source.	*/
			std::vector<char> vertex_refrection_source =
				IOUtil::readFileString(this->vertexRefrectionShaderPath, this->getFileSystem());
			std::vector<char> fragment_refrection_source =
				IOUtil::readFileString(this->fragmentRefrectionShaderPath, this->getFileSystem());

			std::vector<char> vertex_source =
				IOUtil::readFileString(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				IOUtil::readFileString(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->refrection_program =
				ShaderLoader::loadGraphicProgram(&vertex_refrection_source, &fragment_refrection_source);
			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->refrection_program);
			this->uniform_refrection_buffer_index =
				glGetUniformBlockIndex(this->refrection_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->refrection_program, "ReflectionTexture"), 0);
			glUniformBlockBinding(this->refrection_program, this->uniform_refrection_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->reflection_texture = textureImporter.loadImage2D(this->panoramicPath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

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
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	UV.	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			/*	Normal.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
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
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			glBindVertexArray(0);

			/*  */
			glGenFramebuffers(1, &this->HDRFramebuffer);
			glGenTextures(1, &this->shadowTexture);
			onResize(this->width(), this->height());
		}
		virtual void onResize(int width, int height) override {

			this->shadowWidth = width;
			this->shadowHeight = height;
			{
				/*	Create HDR framebuffer.	*/
				glBindFramebuffer(GL_FRAMEBUFFER, this->HDRFramebuffer);

				/*	*/
				glGenTextures(1, &this->shadowTexture);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, this->shadowWidth, this->shadowHeight, 0, GL_RGBA,
							 GL_UNSIGNED_BYTE, nullptr);
				/*	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->shadowTexture, 0);

				/*	*/
				glGenTextures(1, &this->depthTexture);
				glBindTexture(GL_TEXTURE_2D, depthTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->shadowWidth, this->shadowHeight, 0,
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
					throw RuntimeException("Failed to create framebuffer, {}", glewGetErrorString(frstat));
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
		}

		virtual void draw() override {

			this->update();
			int width, height;
			getSize(&width, &height);

			this->uniformBuffer.proj =
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
				glPolygonMode(GL_FRONT_AND_BACK, this->refrectionSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

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
				glPolygonMode(GL_FRONT_AND_BACK, refrectionSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->torus.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	Blit HDR framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->HDRFramebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBlitFramebuffer(0, 0, this->shadowWidth, this->shadowHeight, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
							  GL_NEAREST);
		}

		void update() {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.lookDirection =
				glm::vec4(this->camera.getLookDirection().x, this->camera.getLookDirection().z,
						  this->camera.getLookDirection().y, 0);
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			this->uniformBuffer.position =
				glm::vec4(this->camera.getPosition().x, this->camera.getPosition().z, this->camera.getPosition().y, 0);

			/*  */
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	}; // namespace glsample

	class HDRGLSample : public GLSample<HDR> {
	  public:
		HDRGLSample(int argc, const char **argv) : GLSample<HDR>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Refrection-Sample")("T,texture", "Texture Path",
													 cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		glsample::HDRGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}