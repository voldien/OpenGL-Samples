#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Texture : public GLSampleWindow {
	  public:
		Texture() : GLSampleWindow() { this->setTitle("Texture"); }
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		/*	*/
		unsigned int vao;
		unsigned int vbo;

		int texture_program;
		int gl_texture;

		struct UniformBufferBlock {
			glm::mat4 modelViewProjection;
		} uniform_stage_buffer;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		glm::mat4 proj;
		CameraController camera;

		std::string texturePath = "asset/texture.png";
		/*	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag";

		// TODO add square.
		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->texture_program);

			glDeleteTextures(1, (const GLuint *)&this->gl_texture);

			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
		}

		virtual void Initialize() override {

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			/*	Load shader	*/
			this->texture_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->texture_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->texture_program, "UniformBufferBlock");
			glUniformBlockBinding(this->texture_program, this->uniform_buffer_index, 0);
			glUniform1iARB(glGetUniformLocation(this->texture_program, "diffuse"), 0);
			glUseProgram(0);

			// Load Texture
			this->gl_texture = TextureImporter::loadImage2D(this->texturePath);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize += minMapBufferSize - (uniformSize % minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			this->update();

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			camera.update(getTimer().deltaTime());
			this->proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(this->texture_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->gl_texture);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		virtual void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniform_stage_buffer.modelViewProjection = (this->proj * camera.getViewMatrix());

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *p =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(p, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class TextureGLSample : public GLSample<Texture> {
	  public:
		TextureGLSample(int argc, const char **argv) : GLSample<Texture>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::TextureGLSample sample(argc, argv);
		sample.run();
	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
