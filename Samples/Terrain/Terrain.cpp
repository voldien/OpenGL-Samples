#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class Terrain : public GLSampleWindow {
	  public:
		Terrain() : GLSampleWindow() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			alignas(16) glm::mat4 normalMatrix;

			/*light source.	*/
			glm::vec3 direction = glm::vec3(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4);
		} mvp;

		unsigned int skybox_vbo;
		unsigned vao;
		unsigned int skybox_program;
		unsigned int terrain_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		int skybox_panoramic;
		int terrain_heightMap;
		std::string panoramicPath = "panoramic.jpg";
		CameraController camera;

		const std::string vertexShaderPath = "Shaders/terrain/terrain.vert";
		const std::string fragmentShaderPath = "Shaders/terrain/terrain.frag";

		const std::vector<Vertex> vertices = {{-1.0f, -1.0f, -1.0f, 0, 0}, // triangle 1 : begin
											  {-1.0f, -1.0f, 1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 1, 1}, // triangle 1 : end
											  {1.0f, 1.0f, -1.0f, 1, 1}, // triangle 2 : begin
											  {-1.0f, -1.0f, -1.0f, 1, 0},
											  {-1.0f, 1.0f, -1.0f, 0, 0}, // triangle 2 : end
											  {1.0f, -1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, -1.0f, 0, 1},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {1.0f, 1.0f, -1.0f, 0, 0},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {-1.0f, -1.0f, -1.0f, 1, 0},
											  {-1.0f, -1.0f, -1.0f, 0, 0},
											  {-1.0f, 1.0f, 1.0f, 0, 1},
											  {-1.0f, 1.0f, -1.0f, 1, 1},
											  {1.0f, -1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, 1.0f, 1, 1},
											  {-1.0f, -1.0f, -1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, 1.0f, 0, 1},
											  {1.0f, -1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {1.0f, 1.0f, -1.0f, 1, 0},
											  {1.0f, -1.0f, -1.0f, 0, 0},
											  {1.0f, 1.0f, 1.0f, 0, 1},
											  {1.0f, -1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {1.0f, 1.0f, -1.0f, 1, 1},
											  {-1.0f, 1.0f, -1.0f, 0, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, 1.0f, -1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, 1.0f, 1.0f, 1, 1},
											  {1.0f, -1.0f, 1.0f, 1, 0}

		};

		virtual void Release() override {
			glDeleteProgram(this->skybox_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->skybox_vbo);
			// glDeleteTextures(1, &this->skybox_panoramic);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			/*	Load shader	*/

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			this->skybox_panoramic = TextureImporter::loadImage2D(this->panoramicPath);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->skybox_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, this->skybox_vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glBindVertexArray(0);
		}

		virtual void draw() override {

			camera.update(getTimer().deltaTime());

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Draw terrain.	*/
			glUseProgram(this->terrain_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic);

			glBindVertexArray(this->vao);
			glDrawElements(GL_TRIANGLES, this->vertices.size(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			/*	Draw Skybox.	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			// glEnable(GL_DEPTH_TEST);
			// TODO disable depth write.
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);

			glUseProgram(this->skybox_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		virtual void update() {}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Terrain> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
