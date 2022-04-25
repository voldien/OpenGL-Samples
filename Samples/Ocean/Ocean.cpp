#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class Ocean : public GLWindow {
	  public:
		Ocean() : GLWindow(-1, -1, -1, -1) { this->setTitle("Ocean"); }
		typedef struct _vertex_t {
			float h0[2];
			float ht_real_img[2];
		} Vertex;

		/*	*/
		unsigned int skybox_vao;
		unsigned int skybox_vbo;
		/*	*/
		unsigned int ocean_vao;
		unsigned int ocean_vbo;
		unsigned int ocean_ibo;

		/*	*/
		unsigned int skybox_texture;

		/*	*/
		unsigned int skybox_program;
		unsigned int ocean_graphic_program;
		unsigned int spectrum_compute_program;
		unsigned int kff_compute_program;

		unsigned int ocean_width, ocean_height;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			glm::vec4 diffuseColor;
			float delta;
			/*	*/
			float speed;

		} mvp;

		// TODO change to vector
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;

		const std::string vertexSkyboxShaderPath = "Shaders/skybox-panoramic/skybox.vert";
		const std::string fragmentSkyboxShaderPath = "Shaders/skybox-panoramic/skybox-panoramic.frag";

		const std::string vertexShaderPath = "Shaders/ocean/ocean.vert";
		const std::string fragmentShaderPath = "Shaders/ocean/ocean.frag";
		const std::string tesscShaderPath = "Shaders/ocean/ocean.tesc";
		const std::string teseShaderPath = "Shaders/ocean/ocean.tese";

		const std::string computeShaderPath = "Shaders/ocean/ocean.comp";
		const std::string computeKFFShaderPath = "Shaders/ocean/kff.comp";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->ocean_graphic_program);
			glDeleteProgram(this->spectrum_compute_program);
			glDeleteProgram(this->kff_compute_program);
			/*	*/
			glDeleteBuffers(1, &this->ocean_vbo);
			glDeleteBuffers(1, &this->ocean_ibo);
			glDeleteBuffers(1, &this->skybox_vbo);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);
			std::vector<char> control_source = IOUtil::readFile(tesscShaderPath);
			std::vector<char> evolution_source = IOUtil::readFile(teseShaderPath);

			/*	*/
			std::vector<char> compute_spectrum_source = IOUtil::readFile(computeShaderPath);
			std::vector<char> compute_kff_source = IOUtil::readFile(computeKFFShaderPath);

			/*	Load graphic program	*/
			this->ocean_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Load compute shader program.	*/
			this->spectrum_compute_program = ShaderLoader::loadComputeProgram({&compute_spectrum_source});
			this->kff_compute_program = ShaderLoader::loadComputeProgram({&compute_kff_source});

			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferBlock) * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			// TODO.
			/*	Load geometry.	*/

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->ocean_vao);
			glBindVertexArray(this->ocean_vao);

			glGenBuffers(1, &this->ocean_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, ocean_vbo);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

			glGenBuffers(1, &this->ocean_ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ocean_ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glEnableVertexAttribArrayARB(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glBindVertexArray(0);

			glUseProgram(this->ocean_graphic_program);
			glUniform1iARB(glGetUniformLocation(this->ocean_graphic_program, "reflection"), 0);
			glUniformBlockBinding(this->ocean_graphic_program, glGetUniformLocation(this->ocean_graphic_program, "MVP"),
								  0);
			glUseProgram(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_buffer, sizeof(UniformBufferBlock),
							  sizeof(UniformBufferBlock));

			/*	*/
			glUseProgram(this->spectrum_compute_program);
			glBindVertexArray(this->ocean_vao);
			glDispatchCompute(ocean_width + 64, ocean_height, 1);

			int FFT_SIZE = 0;
			glUseProgram(this->kff_compute_program);
			glDispatchCompute(FFT_SIZE * 64, 1, 1);
			glDispatchCompute(256 * 257 / 2 * 64, 1, 1);
			glDispatchCompute(FFT_SIZE * 64, 1, 1);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_texture);

			glUseProgram(this->ocean_graphic_program);

			/*	Draw triangle*/
			glBindVertexArray(this->ocean_vao);
			//	glDrawArrays(GL_PATCHES, 0, this->vertices.size());
			glBindVertexArray(0);

			// Draw skybox
			glUseProgram(this->skybox_program);
			glBindVertexArray(this->skybox_vao);
			//	glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);
		}

		virtual void update() {}
	};
} // namespace glsample

// TODO param, ocean with,height, skybox

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Ocean> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
