#include "GLSampleWindow.h"

#include "ShaderLoader.h"
#include <FontFactory.h>
#include <GL/glew.h>

#include <iostream>
namespace glsample {

	class Font : public GLSampleWindow {
	  public:
		Font() : GLSampleWindow() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		unsigned int vbo;
		unsigned int vao;

		unsigned int triangle_program;
		const std::string vertexShaderPath = "Shaders/font/font.vert";
		const std::string fragmentShaderPath = "Shaders/font/font.frag";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->triangle_program);
			glDeleteBuffers(1, &this->vbo);
		}
		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			// Load font.
			// FontFactory::

			/*	Load shader	*/
			this->triangle_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->triangle_program);
			this->triangle_program = glGetUniformLocation(this->triangle_program, "MVP");
			glUniform1iARB(glGetUniformLocation(this->triangle_program, "fontatlast"), 0);
			glUseProgram(0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	Vertex.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	UV coordinate.	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);
						glDisable(GL_CULL_FACE);
			//glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);


			glUseProgram(this->triangle_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->triangle_program);

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
		GLSample<glsample::Font> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}