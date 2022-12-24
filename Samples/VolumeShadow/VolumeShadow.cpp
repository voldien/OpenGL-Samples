#include "GLSampleWindow.h"

#include "Importer/ImageImport.h"
#include "ShaderLoader.h"
#include "Util/CameraController.h"
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class VolumeShadow : public GLSampleWindow {
	  public:
		VolumeShadow() : GLSampleWindow() {}
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;
		/*	*/
		GeometryObject plan;

		int volumeshadow_program;
		int gl_texture;
		int mvp_uniform;

		glm::mat4 mvp;
		CameraController camera;

		std::string texturePath = "texture.png";
		/*	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag";

		// TODO add square.
		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->volumeshadow_program);

			glDeleteTextures(1, (const GLuint *)&this->gl_texture);

			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			std::vector<char> vertex_source = IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source = IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->volumeshadow_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			glUseProgram(this->volumeshadow_program);
			this->mvp_uniform = glGetUniformLocation(this->volumeshadow_program, "MVP");
			glUniform1iARB(glGetUniformLocation(this->volumeshadow_program, "diffuse"), 0);
			glUseProgram(0);

			// Load Texture
			TextureImporter textureImporter(this->getFileSystem());
			this->gl_texture = textureImporter.loadImage2D(this->texturePath);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateTorus(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->plan.nrIndicesElements = indices.size();

			/*	Vertex.	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	UV.	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			/*	Normal.	*/
			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(20));

			/*	Tangent.	*/
			glEnableVertexAttribArrayARB(3);
			glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(32));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			this->update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			// Stencil
			{
				glEnable(GL_STENCIL_TEST);

				glDepthMask(GL_FALSE);
				glEnable(GL_DEPTH_CLAMP);
				glDisable(GL_CULL_FACE);

				glStencilFunc(GL_ALWAYS, 0, 0xff);

				// Set the stencil test per the depth fail algorithm
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

				// Draw camera
				glUseProgram(this->volumeshadow_program);

				glDisable(GL_DEPTH_CLAMP);
				glEnable(GL_CULL_FACE);
			}

			{

				glDisable(GL_STENCIL_TEST);

				glUseProgram(this->volumeshadow_program);
				glUniformMatrix4fv(this->mvp_uniform, 1, GL_FALSE, &camera.getViewMatrix()[0][0]);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->gl_texture);

				/*	Draw triangle	*/
				glBindVertexArray(this->plan.vao);
				glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
				glBindVertexArray(0);
			}

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		virtual void update() { camera.update(getTimer().deltaTime()); }
	};

	class VolumeShadowGLSample : public GLSample<VolumeShadow> {
	  public:
		VolumeShadowGLSample(int argc, const char **argv) : GLSample<VolumeShadow>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::VolumeShadowGLSample sample(argc, argv);
		sample.run();
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
