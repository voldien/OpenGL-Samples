#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>

#include <iostream>

namespace glsample {

	class PNTessellation : public GLWindow {
	  public:
		PNTessellation() : GLWindow(-1, -1, -1, -1) {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		/*	*/
		unsigned int vbo;
		unsigned int vao;
		/*	*/
		unsigned int tessellation_program;
		unsigned int mvp_uniform;

		CameraController camera;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int height_map;

		/*	*/
		const std::string vertexShaderPath = "Shaders/pntessellation/pntessellation.vert";
		const std::string fragmentShaderPath = "Shaders/pntessellation/pntessellation.frag";
		const std::string ControlShaderPath = "Shaders/pntessellation/pntessellation.tesc";
		const std::string EvoluationShaderPath = "Shaders/pntessellation/pntessellation.tese";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->tessellation_program);

			/**/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->height_map);
			/*	*/
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);

		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);
			std::vector<char> control_source = IOUtil::readFile(ControlShaderPath);
			std::vector<char> evolution_source = IOUtil::readFile(EvoluationShaderPath);

			/*	Load shader	*/
			this->tessellation_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, nullptr,
																   &control_source, &evolution_source);

			// fragcore::ProceduralGeometry::generatePlan();

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

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(12));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			camera.update(getTimer().deltaTime());
			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(this->tessellation_program);
			glUniformMatrix4fv(this->mvp_uniform, 1, GL_FALSE, &camera.getViewMatrix()[0][0]);

			/**/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/**/
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, this->height_map);

			// Draw split.
			// WireFrame.

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
		GLSample<glsample::PNTessellation> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}