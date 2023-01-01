#include "GLSampleWindow.h"

#include "ShaderLoader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class NBodySimulation : public GLSampleWindow {
	  public:
		NBodySimulation() : GLSampleWindow() {
			this->setTitle("Particle System");
			com = std::make_shared<NBodySimulationSettingView>(this->uniform_stage.settings);
			this->addUIComponent(com);
		}

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;

		/*	Particle Buffer.    */
		static const unsigned int localInvoke = 32;
		static const unsigned int nrParticles = localInvoke * 256;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = nrParticles * sizeof(Particle);

		/*	*/
		unsigned int vao_particle;
		unsigned int vbo_particle;

		/*	*/
		unsigned int particle_texture;

		/*	*/
		unsigned int particle_graphic_program;
		unsigned int particle_compute_program;

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	*/
			typedef struct particle_setting_t {

				int _GroupDim[4] = {256, 1, 1, 1};
				int _ThreadDim[4] = {localInvoke, 1, 1, 1};
				float speed = 0.4f;

				float _SofteningSquared = 0.2f;
				float _DeltaTime = 1.0f;
				float _Damping = 0.01f;
				uint32_t _NumBodies = nrParticles;

			} particleSetting;

			particleSetting settings;

		} uniform_stage;

		class NBodySimulationSettingView : public nekomimi::UIComponent {
		  private:
		  public:
			NBodySimulationSettingView(UniformBufferBlock::particle_setting_t &settings) : settings(settings) {
				this->setName("NBodySimulation Setting");
			}
			virtual void draw() override {

				ImGui::DragFloat("Damping", &this->settings._Damping, 1.0f, 0.0f);
				ImGui::DragFloat("Speed", &this->settings.speed, 1.0f, 0.0f);
			}
			UniformBufferBlock::particle_setting_t &settings;
		};
		std::shared_ptr<NBodySimulationSettingView> com;

		CameraController camera;

		/*	*/
		int particle_buffer_read_index;
		int particle_buffer_write_index;
		int particle_uniform_buffer_index;

		// TODO change to vector
		int particle_graphic_uniform_buffer_index;

		/*	*/
		int uniform_buffer_binding = 0;
		int particle_read_buffer_binding = 1;
		int particle_write_buffer_binding = 2;

		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		const std::string vertexShaderPath = "Shaders/nbodysimulation/nbodysimulation.vert";
		const std::string fragmentShaderPath = "Shaders/nbodysimulation/nbodysimulation.frag";
		const std::string computeShaderPath = "Shaders/nbodysimulation/nbodysimulation.comp";

		virtual void Release() override {

			/*	*/
			glDeleteProgram(this->particle_graphic_program);
			glDeleteProgram(this->particle_compute_program);

			/*	*/
			glDeleteBuffers(1, &this->vbo_particle);
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteVertexArrays(1, &this->vao_particle);

			/*	*/
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source = glsample::IOUtil::readFileString(vertexShaderPath);
			std::vector<char> fragment_source = glsample::IOUtil::readFileString(fragmentShaderPath);

			/*	*/
			std::vector<char> compute_source = glsample::IOUtil::readFileString(computeShaderPath);

			/*	Load shader	*/
			this->particle_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
			this->particle_compute_program = ShaderLoader::loadComputeProgram({&compute_source});

			/*	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1iARB(glGetUniformLocation(this->particle_graphic_program, "diffuse"), 0);
			this->particle_graphic_uniform_buffer_index =
				glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, this->particle_graphic_uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->particle_compute_program);
			this->particle_uniform_buffer_index =
				glGetUniformBlockIndex(this->particle_compute_program, "UniformBufferBlock");
			this->particle_buffer_read_index = glGetUniformBlockIndex(this->particle_compute_program, "ReadBuffer");
			this->particle_buffer_write_index = glGetUniformBlockIndex(this->particle_compute_program, "WriteBuffer");
			// TODO fix
			this->particle_buffer_read_index = 1;
			this->particle_buffer_write_index = 2;

			glUniformBlockBinding(this->particle_compute_program, this->particle_uniform_buffer_index,
								  this->uniform_buffer_binding);
			glShaderStorageBlockBinding(this->particle_compute_program, this->particle_buffer_read_index,
										this->particle_read_buffer_binding);
			glShaderStorageBlockBinding(this->particle_compute_program, this->particle_buffer_write_index,
										this->particle_write_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize += minMapBufferSize - (uniformBufferSize % minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			glGenBuffers(1, &this->vbo_particle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo_particle);
			glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleMemorySize * nrParticleBuffers, nullptr, GL_DYNAMIC_DRAW);

			/*	*/
			Particle *particle_buffer =
				(Particle *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, ParticleMemorySize * nrParticleBuffers,
											 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
			for (int i = 0; i < nrParticles * nrParticleBuffers; i++) {
				particle_buffer[i].position =
					glm::vec4(Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f);
				particle_buffer[i].velocity =
					glm::vec4(Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f);
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao_particle);
			glBindVertexArray(this->vao_particle);

			glBindBuffer(GL_ARRAY_BUFFER, this->vbo_particle);
			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr);

			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
								  reinterpret_cast<void *>(sizeof(float) * 4));

			glBindVertexArray(0);

			fragcore::resetErrorFlag();

			this->uniform_stage.proj =
				glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.15f, 1000.0f);
		}

		virtual void draw() override {

			this->update();

			int width, height;
			getSize(&width, &height);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->particle_uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	*/
			glUseProgram(this->particle_compute_program);

			glBindVertexArray(this->vao_particle);

			/*	*/
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_read_index, this->vbo_particle,
							  ((this->getFrameCount() + 1) % nrParticleBuffers) * ParticleMemorySize,
							  ParticleMemorySize);

			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_write_index, this->vbo_particle,
							  (this->getFrameCount() % nrParticleBuffers) * ParticleMemorySize, ParticleMemorySize);

			/*	*/
			glDispatchCompute(nrParticles / localInvoke, 1, 1);

			glBindVertexArray(0);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, width, height);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->particle_texture);

			glUseProgram(this->particle_graphic_program);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glEnable(GL_PROGRAM_POINT_SIZE_ARB);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
			glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
			glPointParameterf(GL_POINT_SIZE_MIN_ARB, 1.0f);
			glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f);

			/*	Draw triangle*/
			glBindVertexArray(this->vao_particle);

			glBindVertexBuffer(0, this->vbo_particle, (this->getFrameCount() % nrParticleBuffers) * ParticleMemorySize,
							   sizeof(Particle));
			glBindVertexBuffer(1, this->vbo_particle, (this->getFrameCount() % nrParticleBuffers) * ParticleMemorySize,
							   sizeof(Particle));

			glDrawArrays(GL_POINTS, 0, nrParticles);

			glBindVertexArray(0);

			glUseProgram(0);
		}

		void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->uniform_stage.settings._DeltaTime = getTimer().deltaTime();

			this->uniform_stage.model = glm::mat4(1.0f);
			this->uniform_stage.view = camera.getViewMatrix();
			this->uniform_stage.modelViewProjection =
				this->uniform_stage.proj * this->uniform_stage.view * this->uniform_stage.model;

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

			memcpy(uniformPointer, &this->uniform_stage, sizeof(uniform_stage));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::NBodySimulation> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
