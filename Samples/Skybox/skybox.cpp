#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <iostream>

class SkyBoxWindow : public GLWindow {
  public:
	SkyBoxWindow() : GLWindow(-1, -1, -1, -1) {}
	typedef struct _vertex_t {
		float pos[2];
		float color[3];
	} Vertex;
	unsigned int vbo;
	unsigned vao;
	unsigned int skybox_program;
	const std::string vertexShaderPath = "Shaders/skybox/base.vert";
	const std::string fragmentShaderPath = "Shaders/skybox/base.frag";

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
		glDeleteBuffers(1, &this->vbo);
	}
	virtual void Initialize() override {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		/*	Load shader	*/

		std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
		std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

		this->skybox_program = ShaderLoader::loadProgram(&vertex_source, &fragment_source);

		/*	Create array buffer, for rendering static geometry.	*/
		glGenVertexArrays(1, &this->vao);
		glBindVertexArray(this->vao);

		glEnableVertexAttribArrayARB(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

		glGenBuffers(1, &this->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	virtual void draw() override {

		int width, height;
		getSize(&width, &height);

		/*	*/
		glViewport(0, 0, width, height);

		/*	*/
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(this->skybox_program);

		/*	Draw triangle*/
		glBindVertexArray(this->vao);
		glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
	}

	virtual void update() {}
};

int main(int argc, const char **argv) {
	try {

		//	OpenGLCore core(argc, argv);
		SkyBoxWindow w;

		w.run();
	} catch (std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
