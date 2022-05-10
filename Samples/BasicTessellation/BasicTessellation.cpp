#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <GLWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SampleComponent : public nekomimi::UIComponent {
	  private:
	  public:
		SampleComponent() { this->setName("Sample Window"); }
		virtual void draw() override {

			ImGui::ColorEdit4("color 1", color);
			if (ImGui::Button("Press me")) {
			}
		}
		float color[4];
	};

	class BasicTessellation : public GLSampleWindow {
	  public:
		std::shared_ptr<SampleComponent> com;
		BasicTessellation() : GLSampleWindow() {
			this->setTitle("Basic Tessellation");
			com = std::make_shared<SampleComponent>();
			this->addUIComponent(com);
		}
		typedef struct _vertex_t {
			float vertex[3];
			float uv[2];
			float normal[3];
			float tangent[3];
		} Vertex;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 normalMatrix;

			/*light source.	*/
			glm::vec3 direction = glm::vec3(0, 1, 0);
			glm::vec4 lightColor = glm::vec4(1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);

			/*	*/
			glm::vec3 eyePos = glm::vec3(1.0f);
			float gDisplace = 1.0f;
		} mvp;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		/*	*/
		unsigned int vbo;
		unsigned int ibo;
		unsigned int vao;
		bool split = false;
		/*	*/
		unsigned int tessellation_program;

		CameraController camera;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int heightmap_texture;

		const std::string diffuseTexturePath = "tessellation_diffusemap.png";
		const std::string heightTexturePath = "tessellation_heightmap.png";
		/*	*/
		const std::string vertexShaderPath = "Shaders/tessellation/tessellation.vert";
		const std::string fragmentShaderPath = "Shaders/tessellation/tessellation.frag";
		const std::string ControlShaderPath = "Shaders/tessellation/tessellation.tesc";
		const std::string EvoluationShaderPath = "Shaders/tessellation/tessellation.tese";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->tessellation_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->heightmap_texture);

			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);
			std::vector<char> control_source = IOUtil::readFile(ControlShaderPath);
			std::vector<char> evolution_source = IOUtil::readFile(EvoluationShaderPath);

			/*	Load shader	*/
			this->tessellation_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, nullptr,
																		  &control_source, &evolution_source);

			/*	*/
			glUseProgram(this->tessellation_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->tessellation_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->tessellation_program, "diffuse"), 0);
			glUniform1iARB(glGetUniformLocation(this->tessellation_program, "heightmap"), 1);
			glUniformBlockBinding(this->tessellation_program, glGetUniformLocation(this->tessellation_program, "mvp"),
								  0);

			glUseProgram(0);

			/*	Load Diffuse and Height Map Texture.	*/
			this->diffuse_texture = TextureImporter::loadImage2D(this->diffuseTexturePath);
			this->heightmap_texture = TextureImporter::loadImage2D(this->heightTexturePath);

			/*	Load geometry.	*/
			// fragcore::ProceduralGeometry::generatePlan();

			GLint minMapBufferSize;
			glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &minMapBufferSize);
			uniformSize += uniformSize % minMapBufferSize;

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(12));

			/*	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(20));

			/*	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(32));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			update();

			int width, height;
			getSize(&width, &height);

			if (split) {
			}
			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glUseProgram(this->tessellation_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	*/
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, this->heightmap_texture);

			// Draw split.
			// if (split) {
			// 	glScissor(0, 0, width / 2.0, height);
			// }
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
			glDrawArrays(GL_PATCHES, 0, this->vertices.size());

			/*	Draw wireframe outline.	*/
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// glDrawArrays(GL_PATCHES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		virtual void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model =
				glm::rotate(this->mvp.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(0.95f));
			this->mvp.modelViewProjection = camera.getViewMatrix();

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p = glMapBufferRange(GL_UNIFORM_BUFFER, ((getFrameCount() + 1) % nrUniformBuffer) * uniformSize,
									   uniformSize, GL_MAP_WRITE_BIT);
			memcpy(p, &this->mvp, sizeof(mvp));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

// TODO add custom image support.

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::BasicTessellation> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}