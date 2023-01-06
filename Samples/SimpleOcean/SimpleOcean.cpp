
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SimpleOcean : public GLSampleWindow {
	  public:
		SimpleOcean() : GLSampleWindow() {
			this->setTitle("Simple Ocean");
			this->simpleOceanSettingComponent = std::make_shared<SimpleOceanSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->simpleOceanSettingComponent);
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

			float time;
			float speed;
			float freq = 2.0f;
			float amplitude = 1.0f;

		} uniformBuffer;

		/*	*/
		GeometryObject plan;
		GeometryObject skybox;

		/*	*/
		unsigned int normal_texture;
		unsigned int reflection_texture;

		/*	*/
		unsigned int simpleOcean_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Simple Ocean Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("");
				ImGui::DragFloat("Speed", &this->uniform.speed);
				ImGui::DragFloat("Freqency", &this->uniform.freq);
				ImGui::DragFloat("Amplitude", &this->uniform.amplitude);
				ImGui::TextUnformatted("");
				/*	*/
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleOceanSettingComponent;

		CameraController camera;

		std::string panoramicPath = "asset/panoramic.jpg";
		std::string normalTexturePath = "asset/normalmap.png";

		const std::string vertexShaderPath = "Shaders/simpleocean/simpleocean.vert.spv";
		const std::string fragmentShaderPath = "Shaders/simpleocean/simpleocean.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->simpleOcean_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->reflection_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			/*	Load shader source.	*/
			std::vector<char> vertex_simple_ocean_source =
				IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_simple_ocean_source =
				IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());
			std::vector<char> vertex_source =
				IOUtil::readFileString(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				IOUtil::readFileString(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			/*	Load shader programs.	*/
			this->simpleOcean_program =
				ShaderLoader::loadGraphicProgram(&vertex_simple_ocean_source, &fragment_simple_ocean_source);
			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->simpleOcean_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->simpleOcean_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->simpleOcean_program, "ReflectionTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->simpleOcean_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->simpleOcean_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->normal_texture = textureImporter.loadImage2D(this->normalTexturePath);
			this->reflection_texture = textureImporter.loadImage2D(this->panoramicPath);

			/*	Create Normal texture.	*/

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, 256, 256);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*  Create index buffer.    */
			glGenBuffers(1, &this->plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->plan.nrIndicesElements = indices.size();

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
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				glUseProgram(this->simpleOcean_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, simpleOceanSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
			/*	*/
			{
				glUseProgram(this->simpleOcean_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, simpleOceanSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
			// Skybox
		}

		void update() {
			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

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
			std::cout << this->uniformBuffer.position.x << std::endl;
			this->uniformBuffer.time = elapsedTime;

			/*  */
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(uniformBuffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	}; // namespace glsample

	class SimpleOceanGLSample : public GLSample<SimpleOcean> {
	  public:
		SimpleOceanGLSample(int argc, const char **argv) : GLSample<SimpleOcean>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

// TODO add custom options.
int main(int argc, const char **argv) {
	try {
		glsample::SimpleOceanGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}