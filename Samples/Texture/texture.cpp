#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>
#include <iostream>
namespace glsample {

	class Texture : public GLWindow {
	  public:
		Texture() : GLWindow(-1, -1, -1, -1) {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		unsigned int vao;
		unsigned int vbo;
		int texture_program;
		int gl_texture;
		int mvp_uniform;

		glm::mat4 mvp;
		CameraController camera;
		std::string texturePath = "texture.png";
		const std::string vertexShaderPath = "Shaders/texture/texture.vert";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag";

		// TODO add square.
		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->texture_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);

			glDeleteTextures(1, (const GLuint *)&this->gl_texture);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			/*	Load shader	*/
			this->texture_program = ShaderLoader::loadProgram(&vertex_source, &fragment_source);

			glUseProgram(this->texture_program);
			this->mvp_uniform = glGetUniformLocation(this->texture_program, "MVP");
			glUniform1iARB(glGetUniformLocation(this->texture_program, "diffuse"), 0);
			glUseProgram(0);

			// Load Texture
			this->gl_texture = TextureImporter::loadImage2D(this->texturePath);

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

			camera.update(getTimer().deltaTime());

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(this->texture_program);
			glUniformMatrix4fv(this->mvp_uniform, 1, GL_FALSE, &camera.getViewMatrix()[0][0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->gl_texture);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);

			// Draw IMGUI
		}

		virtual void update() {}
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

		std::cerr << cxxexcept::getStackMessage(ex);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
