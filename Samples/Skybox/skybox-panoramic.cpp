#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ImageImport.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class SkyBoxPanoramic : public GLWindow {
	  public:
		SkyBoxPanoramic() : GLWindow(-1, -1, -1, -1) {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		unsigned int vbo;
		unsigned vao;
		unsigned int skybox_program;
		glm::mat4 mvp;
		int skybox_panoramic;
		std::string panoramicPath = "panoramic.jpg";
		CameraController camera;

		unsigned int mvp_uniform;

		const std::string vertexShaderPath = "Shaders/skybox-panoramic/skybox.vert";
		const std::string fragmentShaderPath = "Shaders/skybox-panoramic/panoramic.frag";

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
			glDeleteTextures(1, (const GLuint *)&this->skybox_panoramic);
		}
		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			/*	Load shader	*/

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->skybox_program);
			this->mvp_uniform = glGetUniformLocation(this->skybox_program, "MVP");
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			this->skybox_panoramic = TextureImporter::loadImage2D(this->panoramicPath);

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
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);

			glUseProgram(this->skybox_program);
			glUniformMatrix4fv(this->mvp_uniform, 1, GL_FALSE, &camera.getViewMatrix()[0][0]);

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
		GLSample<glsample::SkyBoxPanoramic> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
