#include <GL/glew.h>
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

		GeometryObject skybox;
		GeometryObject terrain;

		/*	*/
		unsigned int terrain_vao;
		unsigned int terrain_vbo;
		unsigned int ibo;
		unsigned int nrElements;

		unsigned int skybox_program;
		unsigned int terrain_program;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		int terrain_diffuse_texture;
		int terrain_heightMap;
		std::string panoramicPath = "panoramic.jpg";

		glm::mat4 proj;
		CameraController camera;

		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag";

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

			glDeleteVertexArrays(1, &this->terrain_vao);
			glDeleteBuffers(1, &this->terrain_vbo);
			glDeleteBuffers(1, &this->ibo);

			// glDeleteBuffers(1, &this->skybox_vbo);
			// glDeleteTextures(1, &this->skybox_panoramic);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexTerrainShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentTerrainShaderPath);

			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Create Terrain Shade.	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Create terrain texture.	*/
			TextureImporter textureImporter(FileSystem::getFileSystem());
			this->terrain_diffuse_texture = textureImporter.loadImage2D(this->panoramicPath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize = Math::align(uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(20, vertices, indices, 128, 128);

			//		ProceduralGeometry::generateCube(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->terrain_vao);
			glBindVertexArray(this->terrain_vao);

			glGenBuffers(1, &this->ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->nrElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->terrain_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, terrain_vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

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
			glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

			glBindVertexArray(this->terrain_vao);
			glDrawElements(GL_TRIANGLES, this->vertices.size(), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			/*	Draw Skybox.	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			// glEnable(GL_DEPTH_TEST);
			// TODO disable depth write.
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);

			/*	Draw Skybox.	*/
			glUseProgram(this->skybox_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

			glBindVertexArray(this->terrain_vao);
			glDrawElements(GL_TRIANGLES, this->nrElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		virtual void update() {
			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			// this->proj =
			//	glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			// this->uniform_stage_buffer.modelViewProjection = (this->proj * camera.getViewMatrix());
			//
			// glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			// void *uniformMappedMemory = glMapBufferRange(
			//	GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize, uniformSize,
			//	GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			// memcpy(uniformMappedMemory, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			// glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class TerrainGLSample : public GLSample<Terrain> {
	  public:
		TerrainGLSample(int argc, const char **argv) : GLSample<Terrain>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Terrain> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
