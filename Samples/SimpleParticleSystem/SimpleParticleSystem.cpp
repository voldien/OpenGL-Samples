#include <GLSampleWindow.h>
#include <ShaderLoader.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <Importer/ImageImport.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SimpleParticleSystem : public GLSampleWindow {
	  public:
		SimpleParticleSystem() : GLSampleWindow() {
			this->setTitle("Simple ParticleSystem");
			this->simpleParticleSettingComponent =
				std::make_shared<SimpleParticleSystemSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->simpleParticleSettingComponent);
		}

		/*	*/
		const unsigned int localInvoke = 32;
		unsigned int nrParticles = localInvoke * 256;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = nrParticles * sizeof(Particle);

		/*	*/
		unsigned int vao_particle;
		unsigned int vbo_particle;

		/*	*/
		unsigned int particle_texture;

		/*	*/
		unsigned int particle_graphic_program;

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

		} uniformBuffer;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity.	*/
		} Particle;
		CameraController camera;

		/*	*/
		int particle_read_buffer_binding = 1;
		int particle_write_buffer_binding = 2;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class SimpleParticleSystemSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleParticleSystemSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Simple Particle System Settings");
			}
			virtual void draw() override {
				ImGui::DragFloat("Speed", &this->uniform.particleSetting.speed, 1, 0.0f, 100.0f);
				ImGui::DragFloat("LifeTime", &this->uniform.particleSetting.lifetime, 1, 0.0f, 10.0f);
			}

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SimpleParticleSystemSettingComponent> simpleParticleSettingComponent;

		/*	*/
		const std::string vertexShaderPath = "Shaders/simpleparticlesystem/simpleparticlesystem.vert";
		const std::string fragmentShaderPath = "Shaders/simpleparticlesystem/simpleparticlesystem.frag";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->particle_graphic_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteVertexArrays(1, &this->vao_particle);
			glDeleteBuffers(1, &this->vbo_particle);
		}

		virtual void Initialize() override {

			const std::string particleTexturePath = "asset/particle.png";

			/*	*/
			std::vector<char> vertex_source = glsample::IOUtil::readFileString(vertexShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				glsample::IOUtil::readFileString(fragmentShaderPath, this->getFileSystem());

			/*	Load shader	*/
			this->particle_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->particle_texture = textureImporter.loadImage2D(particleTexturePath);

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1i(glGetUniformLocation(this->particle_graphic_program, "spriteTexture"), 0);
			int uniform_buffer_index = glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align<size_t>(uniformBufferSize, minMapBufferSize);

			/*	Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*  */
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
							  Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * uniformBuffer.particleSetting.lifetime);
				particle_buffer[i].velocity =
					glm::vec4(Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f);
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao_particle);
			glBindVertexArray(this->vao_particle);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_particle);
			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
								  reinterpret_cast<void *>(sizeof(float) * 4));

			glBindVertexArray(0);

			fragcore::resetErrorFlag();
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			{
				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, particle_texture);

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
				glBindVertexBuffer(0, this->vbo_particle,
								   (this->getFrameCount() % nrParticleBuffers) * ParticleMemorySize, sizeof(Particle));
				glBindVertexBuffer(1, this->vbo_particle,
								   (this->getFrameCount() % nrParticleBuffers) * ParticleMemorySize, sizeof(Particle));

				glDrawArrays(GL_POINTS, 0, nrParticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		virtual void update() override {
			/*	*/
			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);

			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->uniformBuffer.delta = getTimer().deltaTime();

			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.view = camera.getViewMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;

			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

			memcpy(uniformPointer, &this->uniformBuffer, sizeof(uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::SimpleParticleSystem> sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
