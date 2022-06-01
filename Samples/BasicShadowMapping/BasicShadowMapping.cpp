#include "GLSampleWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <utils/CameraController.h>

#include <iostream>
namespace glsample {

	class BasicShadowMapping : public GLSampleWindow {
	  public:
		BasicShadowMapping() : GLSampleWindow() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		unsigned int shadowFramebuffer;
		unsigned int shadowTexture;
		unsigned int shadowWidth, shadowHeight;

		unsigned int vao;
		unsigned int vbo;
		unsigned int graphic_program;
		unsigned int shadow_program;

		CameraController camera;

		const std::string vertexShaderPath = "Shaders/triangle/vertex.glsl";
		const std::string fragmentShaderPath = "Shaders/triangle/fragment.glsl";
		const std::string vertexShadowShaderPath = "Shaders/triangle/vertex.glsl";
		const std::string fragmentShadowShaderPath = "Shaders/triangle/fragment.glsl";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteBuffers(1, &this->vbo);
		}
		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			std::vector<char> vertex_shadow_source = IOUtil::readFile(vertexShadowShaderPath);
			std::vector<char> fragment_shadow_source = IOUtil::readFile(fragmentShadowShaderPath);

			/*	Load shader	*/
			this->graphic_program = ShaderLoader::loadProgram(&vertex_source, &fragment_source);
			this->shadow_program = ShaderLoader::loadProgram(&vertex_shadow_source, &fragment_shadow_source);

			/*	Create shadow map.	*/
			glGenFramebuffers(1, &shadowFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
			int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frstat != GL_FRAMEBUFFER_COMPLETE) {
				/*  Delete  */
				glDeleteFramebuffers(1, &glfraobj->framebuffer);
				throw RuntimeException("Failed to create framebuffer, {}.\n", frstat);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	Vertices.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	UVs	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	Normals.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			{
				/*  Draw light source.  */
				glDrawArrays(GL_TRIANGLES, 0, vertices.size());
			}

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(this->graphic_program);

			/**/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/**/
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, this->normal_texture);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);

			/*	Blit shadow map.	*/
			glBlitFramebuffer(0, 0, 0, 0, 0, 0, 0, 0, GL_DEPTH_BUFFER_BIT, GL_LINEAR);
		}

		virtual void update() {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::BasicShadowMapping> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}