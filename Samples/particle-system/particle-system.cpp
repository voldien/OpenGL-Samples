#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class ParticleSystem : public GLWindow {
	  public:
		ParticleSystem() : GLWindow(-1, -1, -1, -1) { this->setTitle("Particle System"); }
		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		/*	*/
		unsigned int vao;
		unsigned int vbo;

		unsigned int particle_texture;

		unsigned int particle_graphic_program;
		unsigned int particle_compute_program;

		unsigned int nrParticles = 1000;
		const unsigned int localInvoke = 32;

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
			float lifetime;
			float gravity;

		} mvp;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;
		CameraController camera;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		const std::string vertexShaderPath = "Shaders/particle/particle.vert";
		const std::string fragmentShaderPath = "Shaders/particle/particle.frag";
		const std::string computeShaderPath = "Shaders/particle/particle.comp";

		const std::vector<Vertex> vertices = {
			{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

		virtual void Release() override {
			glDeleteProgram(this->particle_graphic_program);
			glDeleteBuffers(1, &this->vbo);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source = glsample::IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = glsample::IOUtil::readFile(fragmentShaderPath);

			/*	*/
			std::vector<char> compute_source = glsample::IOUtil::readFile(computeShaderPath);

			/*	Load shader	*/
			this->particle_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
			this->particle_compute_program = ShaderLoader::loadComputeProgram({&compute_source});

			/*	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1iARB(glGetUniformLocation(this->particle_graphic_program, "diffuse"), 0);
			glUseProgram(0);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBufferBlock) * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, nrParticles * sizeof(Particle), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(sizeof(float) * 2));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT);

			glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_buffer, sizeof(UniformBufferBlock),
							  sizeof(UniformBufferBlock));

			glUseProgram(this->particle_compute_program);
			glBindVertexArray(this->vao);
			glDispatchCompute(nrParticles / localInvoke, 1, 1);

			/*	*/
			glViewport(0, 0, width, height);

			glUseProgram(this->particle_graphic_program);
			glEnable(GL_BLEND);
			// glBlendEquationSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA);
			// TODO add blend factor.

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
		GLSample<glsample::ParticleSystem> sample(argc, argv);

		sample.run();

	} catch (std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
