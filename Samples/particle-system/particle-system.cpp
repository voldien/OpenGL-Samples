#include "GLSampleWindow.h"
#include "GLWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class ParticleSystem : public GLSampleWindow {
	  public:
		ParticleSystem() : GLSampleWindow() { this->setTitle("Particle System"); }

		/*	*/
		unsigned int vao;
		unsigned int vbo;

		unsigned int particle_texture;

		unsigned int particle_graphic_program;
		unsigned int particle_compute_program;

		const unsigned int localInvoke = 32;
		unsigned int nrParticles = localInvoke * 64;

		typedef struct particle_setting_t {
			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
		} ParticleSetting;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	*/
			float delta;

			/*	*/
			ParticleSetting particleSetting;

		} mvp;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;
		CameraController camera;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int particle_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int particle_buffer_binding = 1;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		const std::string vertexShaderPath = "Shaders/particle-system/particle.vert";
		const std::string fragmentShaderPath = "Shaders/particle-system/particle.frag";
		const std::string computeShaderPath = "Shaders/particle-system/particle.comp";

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
			this->uniform_buffer_index = glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, this->uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->particle_compute_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->particle_compute_program, "UniformBufferBlock");
			this->particle_buffer_index = glGetUniformBlockIndex(this->particle_compute_program, "Positions");
			this->particle_buffer_index = 1;
			glUniformBlockBinding(this->particle_compute_program, this->uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUniformBlockBinding(this->particle_compute_program, this->particle_buffer_index, particle_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize += uniformBufferSize % minMapBufferSize;

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, nrParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
								  reinterpret_cast<void *>(sizeof(float) * 4));

			glBindVertexArray(0);

			fragcore::resetErrorFlag();

			this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.15f, 1000.0f);
		}

		virtual void draw() override {

			this->update();

			int width, height;
			getSize(&width, &height);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			/*	*/
			glUseProgram(this->particle_compute_program);
			glBindVertexArray(this->vao);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, particle_buffer_index, this->vbo);

			/*	*/
			glDispatchCompute(nrParticles / localInvoke, 1, 1);
			glBindVertexArray(0);
			glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

			/*	*/
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			glUseProgram(this->particle_graphic_program);
			//	glEnable(GL_BLEND);
			// glBlendEquationSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA);
			// TODO add blend factor.
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_POINTS, 0, nrParticles);
			glBindVertexArray(0);

			glUseProgram(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->mvp.delta = getTimer().deltaTime();

			this->mvp.model = glm::mat4(1.0f);
			this->mvp.view = camera.getViewMatrix();
			this->mvp.modelViewProjection = this->mvp.proj * this->mvp.view * this->mvp.model;

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			fragcore::checkError();
			void *p =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformBufferSize,
								 uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			fragcore::checkError();
			memcpy(p, &this->mvp, sizeof(mvp));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
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
