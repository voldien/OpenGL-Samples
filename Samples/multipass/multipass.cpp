#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>

#include <iostream>
namespace glsample {

	class MultiPass : public GLWindow {
	  public:
		MultiPass() : GLWindow(-1, -1, -1, -1) {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		unsigned int vbo;
		unsigned int vao;

		unsigned int fbo;

		unsigned int mandelbrot_framebuffer;
		unsigned int texture_program;
		std::vector<int> gl_textures;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

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

			/*	*/
			glGenFramebuffers(1, &this->mandelbrot_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->mandelbrot_framebuffer);

			glGenTextures(this->gl_textures.size(), &this->gl_textures.data());
			for (size_t i = 0; i < gl_texture.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->gl_texture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width(), this->height(), 0, GL_RGB, GL_UNSIGNED_BYTE,
							 nullptr);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		virtual void onResize(int width, int height) override {
			/*	Resize the image.	*/
			for (size_t i = 0; i < gl_texture.size(); i++) {

				glBindTexture(GL_TEXTURE_2D, this->gl_texture[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width(), this->height(), 0, GL_RGB, GL_UNSIGNED_BYTE,
							 nullptr);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(this->triangle_program);

			/*	Draw triangle*/
			glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());

			/*	Blit image targets to screen.	*/
		}

		virtual void update() {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::MultiPass> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}