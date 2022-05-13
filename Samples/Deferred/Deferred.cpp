#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>

#include <iostream>
namespace glsample {

	class Deferred : public GLWindow {
	  public:
		Deferred() : GLWindow(-1, -1, -1, -1) {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		typedef struct _light_t {

		} Light;
		unsigned int vbo;
		unsigned int triangle_program;
		const std::string vertexShaderPath = "Shaders/triangle/vertex.glsl";
		const std::string fragmentShaderPath = "Shaders/triangle/fragment.glsl";

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

			/*	Load shader	*/
			this->triangle_program = ShaderLoader::loadProgram(&vertex_source, &fragment_source);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(this->triangle_program);

			/*	Draw triangle*/
			glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
		}

		virtual void update() {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Deferred> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}