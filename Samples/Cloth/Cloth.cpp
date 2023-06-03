#include <GLSampleWindow.h>
#include <ShaderLoader.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <Importer/ImageImport.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
namespace glsample {

	class CascadingShadow : public GLSampleWindow {
	  public:
		CascadingShadow() : GLSampleWindow() {
			this->setTitle("CascadingShadow");
			this->vectorFieldSettingComponent = std::make_shared<ParticleSystemSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->vectorFieldSettingComponent);
		}

		/*	*/
		size_t nrParticles = 0;
		const size_t nrParticleBuffers = 3;
		size_t ParticleMemorySize = 0;
		size_t VectorFieldMemorySize = 0;

		/*	*/
		GeometryObject particles;
		GeometryObject grid;
		GeometryObject vectorField;

		unsigned int vao_particle;
		unsigned int vbo_particle;

		/*	*/
		unsigned int particle_texture;
		unsigned int grid_texture;

		int localWorkGroupSize[3];

		/*	*/
		unsigned int particle_graphic_program;
		unsigned int particle_compute_program;
		unsigned int grid_graphic_program;
		unsigned int vector_field_graphic_program;

		typedef struct motion_t {
			glm::vec2 pos; /*  Position in pixel space.    */
			glm::vec2 velocity /*  direction and magnitude of mouse movement.  */;
			float radius; /*  Radius of incluense, also the pressure of input.    */
		} Motion;

		typedef struct particle_setting_t {
			glm::uvec3 particleBox = glm::uvec3(16, 16, 16);
			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;
			float density = 1.0f;
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
			Motion motion;

			glm::vec4 ambientColor;
			glm::vec4 color;

		} uniformBuffer;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity, mass	*/
		} Particle;

		CameraController camera;

		/*	*/
		unsigned int uniform_buffer_particle_graphic_index;
		unsigned int uniform_buffer_particle_compute_index;
		unsigned int uniform_buffer_vector_field_index;

		/*	*/
		int particle_buffer_vector_field_index;
		int particle_buffer_read_index;
		int particle_buffer_write_index;
		int particle_read_buffer_binding = 1;
		int particle_write_buffer_binding = 2;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		class ParticleSystemSettingComponent : public nekomimi::UIComponent {

		  public:
			ParticleSystemSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Particle Settings");
			}
			virtual void draw() override {
				ImGui::DragFloat("Speed", &this->uniform.particleSetting.speed, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Strength", &this->uniform.particleSetting.strength, 1, 0.0f, 100.0f);
				ImGui::DragFloat("LifeTime", &this->uniform.particleSetting.lifetime, 1, 0.0f, 10.0f);
			}

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<ParticleSystemSettingComponent> vectorFieldSettingComponent;

		/*	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";

		/*	*/
		const std::string particleComputeShaderPath = "Shaders/vectorfield/particle.comp.spv";

		/*	*/
		const std::string vectorFieldVertexShaderPath = "Shaders/vectorfield/vectorField.vert.spv";
		const std::string vectorFieldGeometryShaderPath = "Shaders/vectorfield/vectorField.geom.spv";
		const std::string vectorFieldFragmentPath = "Shaders/vectorfield/vectorField.frag.spv";

		/*	*/
		const std::string particleTexturePath = "asset/particle.png";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->particle_graphic_program);
			glDeleteProgram(this->particle_compute_program);

			glDeleteTextures(1, &this->particle_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteVertexArrays(1, &this->vao_particle);
			glDeleteBuffers(1, &this->vbo_particle);
		}

		virtual void Initialize() override {

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	*/
			std::vector<char> vertex_source =
				glsample::IOUtil::readFileString(this->particleVertexShaderPath, this->getFileSystem());
			std::vector<char> geometry_source =
				glsample::IOUtil::readFileString(this->particleGeometryShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				glsample::IOUtil::readFileString(this->particleFragmentShaderPath, this->getFileSystem());

			/*	*/
			std::vector<char> compute_source =
				glsample::IOUtil::readFileString(this->particleComputeShaderPath, this->getFileSystem());

			/*	Load Graphic Program.	*/
			this->particle_graphic_program =
				ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, &geometry_source);
			/*	Load Compute.	*/
			this->particle_compute_program = ShaderLoader::loadComputeProgram({&compute_source});

			/*	*/
			vertex_source = glsample::IOUtil::readFileString(this->vectorFieldVertexShaderPath, this->getFileSystem());
			geometry_source =
				glsample::IOUtil::readFileString(this->vectorFieldGeometryShaderPath, this->getFileSystem());
			fragment_source = glsample::IOUtil::readFileString(this->vectorFieldFragmentPath, this->getFileSystem());

			this->vector_field_graphic_program =
				ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, &geometry_source);

			/*	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->particle_texture = textureImporter.loadImage2D(particleTexturePath);

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1i(glGetUniformLocation(this->particle_graphic_program, "spriteTexture"), 0);
			this->uniform_buffer_particle_graphic_index =
				glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, this->uniform_buffer_particle_graphic_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup particle simulation pipeline.	*/
			glUseProgram(this->particle_compute_program);
			this->uniform_buffer_particle_compute_index =
				glGetUniformBlockIndex(this->particle_compute_program, "UniformBufferBlock");
			this->particle_buffer_vector_field_index =
				glGetUniformBlockIndex(this->particle_compute_program, "VectorField");
			this->particle_buffer_read_index = glGetUniformBlockIndex(this->particle_compute_program, "ReadBuffer");
			this->particle_buffer_write_index = glGetUniformBlockIndex(this->particle_compute_program, "WriteBuffer");
			glGetProgramiv(this->particle_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
			// TODO fix
			this->particle_buffer_read_index = 1;
			this->particle_buffer_write_index = 2;

			glUniformBlockBinding(this->particle_compute_program, this->uniform_buffer_particle_compute_index,
								  this->uniform_buffer_binding);
			/*	*/
			glShaderStorageBlockBinding(this->particle_compute_program, this->particle_buffer_read_index,
										this->particle_read_buffer_binding);
			glShaderStorageBlockBinding(this->particle_compute_program, this->particle_buffer_write_index,
										this->particle_write_buffer_binding);

			glUseProgram(0);

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->vector_field_graphic_program);
			this->uniform_buffer_vector_field_index =
				glGetUniformBlockIndex(this->vector_field_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->vector_field_graphic_program, this->uniform_buffer_vector_field_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align<size_t>(uniformBufferSize, minMapBufferSize);

			// GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Compute number of particles and memory size.	*/
			this->nrParticles =
				(size_t)(uniformBuffer.particleSetting.particleBox.x * uniformBuffer.particleSetting.particleBox.y *
						 uniformBuffer.particleSetting.particleBox.z);
			this->ParticleMemorySize = this->nrParticles * sizeof(Particle);

			/*	Create buffer.	*/
			glGenBuffers(1, &this->vbo_particle);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, vbo_particle);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->ParticleMemorySize * this->nrParticleBuffers, nullptr,
						 GL_DYNAMIC_DRAW);

			/*	*/
			Particle *particle_buffer = static_cast<Particle *>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->ParticleMemorySize * this->nrParticleBuffers,
								 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
			for (size_t i = 0; i < this->nrParticles * this->nrParticleBuffers; i++) {
				particle_buffer[i].position =
					glm::vec4(Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * 100.0f,
							  Random::normalizeRand<float>() * this->uniformBuffer.particleSetting.lifetime);
				particle_buffer[i].velocity = glm::vec4(
					Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
					Random::normalizeRand<float>() * 100.0f, 1.0f / (Random::normalizeRand<float>() * 100.0f));
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

			this->VectorFieldMemorySize = this->nrParticles * sizeof(glm::vec3);

			glGenVertexArrays(1, &this->vectorField.vao);
			glBindVertexArray(this->vectorField.vao);

			glGenBuffers(1, &this->vectorField.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->vectorField.vbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->VectorFieldMemorySize, nullptr, GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

			glBindVertexArray(0);

			fragcore::resetErrorFlag();
		}

		virtual void draw() override {

			int width, height;
			getSize(&width, &height);

			/*	Compute particles in vector field.	*/
			{
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_particle_compute_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_vector_field_index,
								  this->vectorField.vbo, 0, this->VectorFieldMemorySize);

				/*	Bind read particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_read_index, this->vbo_particle,
								  ((this->getFrameCount()) % nrParticleBuffers) * ParticleMemorySize,
								  this->ParticleMemorySize);

				/*	Bind write particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_write_index, this->vbo_particle,
								  ((this->getFrameCount() + 1) % this->nrParticleBuffers) * this->ParticleMemorySize,
								  this->ParticleMemorySize);

				/*	Compute.	*/
				glUseProgram(this->particle_compute_program);
				glDispatchCompute(this->uniformBuffer.particleSetting.particleBox.x / this->localWorkGroupSize[0],
								  this->uniformBuffer.particleSetting.particleBox.y / this->localWorkGroupSize[1],
								  this->uniformBuffer.particleSetting.particleBox.z / this->localWorkGroupSize[2]);

				glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

				glUseProgram(0);
			}

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Draw Vector field.	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_vector_field_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*  Draw grid.  */
				glDisable(GL_BLEND);
				glDisable(GL_CULL_FACE);

				glUseProgram(this->vector_field_graphic_program);

				glLineWidth(3.0f);

				/*	Draw triangle.	*/
				glBindVertexArray(this->vectorField.vao);
				glDrawArrays(GL_POINTS, 0, this->nrParticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Draw Particles.	*/
			{

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_particle_graphic_index, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, particle_texture);

				glUseProgram(this->particle_graphic_program);
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_DEPTH_TEST);

				glEnable(GL_PROGRAM_POINT_SIZE_ARB);
				glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
				glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
				glPointParameterf(GL_POINT_SIZE_MIN_ARB, 1.0f);
				glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f);

				/*	Draw triangle.	*/
				glBindVertexArray(this->vao_particle);
				/*	Bind.	*/
				glBindVertexBuffer(0, this->vbo_particle,
								   ((this->getFrameCount() + 0) % this->nrParticleBuffers) * ParticleMemorySize, 0);
				glBindVertexBuffer(1, this->vbo_particle,
								   ((this->getFrameCount() + 0) % this->nrParticleBuffers) * ParticleMemorySize,
								   sizeof(glm::vec4));
				glDrawArrays(GL_POINTS, 0, this->nrParticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		virtual void update() override {
			/*	*/
			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);

			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			this->uniformBuffer.delta = this->getTimer().deltaTime();

			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::CascadingShadow> sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
