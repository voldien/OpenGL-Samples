#include "GLSampleWindow.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <Importer/ImageImport.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief 
	 * 
	 */
	class VectorField : public GLSampleWindow {
	  public:
		VectorField() : GLSampleWindow() {
			this->setTitle("VectorField");
			this->vectorFieldSettingComponent =
				std::make_shared<ParticleSystemSettingComponent>(this->uniformStageBuffer);
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

		/*	*/
		unsigned int particle_texture;
		unsigned int grid_texture;

		int localWorkGroupSize[3];

		/*	Shader pipeline programs.	*/
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
			glm::uvec4 particleBox = glm::uvec4(16, 16, 16, 0);
			glm::uvec4 vectorfieldbox = glm::uvec4(16, 16, 16, 0);

			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;
			float density = 1.0f;
			float padd0;
			float padd1;
			float padd2;
		} ParticleSetting;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	*/
			ParticleSetting particleSetting;
			Motion motion;

			glm::vec4 color = glm::vec4(1);

			/*	*/
			float delta;

		} uniformStageBuffer;

		typedef struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity, mass	*/
		} Particle;

		typedef struct vector_force_t {
			glm::vec3 position;
			glm::vec3 force;
		} VectorForce;

		CameraController camera;

		/*	*/
		int particle_read_buffer_binding = 1;
		int particle_write_buffer_binding = 2;
		int particle_vectorfield_buffer_binding = 3;

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
			void draw() override {
				ImGui::DragFloat("Speed", &this->uniform.particleSetting.speed, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Strength", &this->uniform.particleSetting.strength, 1, 0.0f, 100.0f);
				ImGui::DragFloat("LifeTime", &this->uniform.particleSetting.lifetime, 1, 0.0f, 10.0f);
				ImGui::ColorEdit4("LifeTime", &this->uniform.color[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				ImGui::Checkbox("Simulate Particles", &this->simulateParticles);
				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool simulateParticles;
			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<ParticleSystemSettingComponent> vectorFieldSettingComponent;

		/*	Particle.	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";

		/*	Particle Simulation in Vector Field.	*/
		const std::string particleComputeShaderPath = "Shaders/vectorfield/particle.comp.spv";

		/*	*/
		const std::string vectorFieldVertexShaderPath = "Shaders/vectorfield/vectorField.vert.spv";
		const std::string vectorFieldGeometryShaderPath = "Shaders/vectorfield/vectorField.geom.spv";
		const std::string vectorFieldFragmentPath = "Shaders/vectorfield/vectorField.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->particle_graphic_program);
			glDeleteProgram(this->particle_compute_program);

			glDeleteTextures(1, &this->particle_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteVertexArrays(1, &this->particles.vao);
			glDeleteBuffers(1, &this->particles.vbo);
		}

		void Initialize() override {

			/*	*/
			const std::string particleTexturePath = this->getResult()["texture"].as<std::string>();

			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	*/
				std::vector<uint32_t> vertex_source =
					glsample::IOUtil::readFileData<uint32_t>(this->particleVertexShaderPath, this->getFileSystem());
				std::vector<uint32_t> geometry_source =
					glsample::IOUtil::readFileData<uint32_t>(this->particleGeometryShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_source =
					glsample::IOUtil::readFileData<uint32_t>(this->particleFragmentShaderPath, this->getFileSystem());

				/*	Load Graphic Program.	*/
				this->particle_graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source,
																				  &fragment_source, &geometry_source);

				/*	*/
				std::vector<uint32_t> compute_source_binary =
					IOUtil::readFileData<uint32_t>(this->particleComputeShaderPath, this->getFileSystem());

				std::vector<char> compute_source =
					fragcore::ShaderCompiler::convertSPIRV(compute_source_binary, compilerOptions);

				/*	Load Compute.	*/
				this->particle_compute_program = ShaderLoader::loadComputeProgram({&compute_source});

				/*	*/
				vertex_source =
					glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldVertexShaderPath, this->getFileSystem());
				geometry_source = glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldGeometryShaderPath,
																		   this->getFileSystem());
				fragment_source =
					glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldFragmentPath, this->getFileSystem());

				this->vector_field_graphic_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_source, &fragment_source, &geometry_source);
			}

			/*	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->particle_texture = textureImporter.loadImage2D(particleTexturePath);

			/*	Setup particle graphic render pipeline.	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1i(glGetUniformLocation(this->particle_graphic_program, "spriteTexture"), 0);
			int uniform_buffer_particle_graphic_index =
				glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, uniform_buffer_particle_graphic_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup particle simulation compute pipeline.	*/
			glUseProgram(this->particle_compute_program);
			int uniform_buffer_particle_compute_index =
				glGetUniformBlockIndex(this->particle_compute_program, "UniformBufferBlock");
			int particle_buffer_vector_field_index =
				glGetProgramResourceIndex(this->particle_compute_program, GL_SHADER_STORAGE_BLOCK, "VectorField");
			int particle_buffer_read_index =
				glGetProgramResourceIndex(this->particle_compute_program, GL_SHADER_STORAGE_BLOCK, "ReadBuffer");
			int particle_buffer_write_index =
				glGetProgramResourceIndex(this->particle_compute_program, GL_SHADER_STORAGE_BLOCK, "WriteBuffer");
			glGetProgramiv(this->particle_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

			// glGetAttribLocation(this->particle_compute_program, )

			/*	*/
			glUniformBlockBinding(this->particle_compute_program, uniform_buffer_particle_compute_index,
								  this->uniform_buffer_binding);
			/*	*/
			glShaderStorageBlockBinding(this->particle_compute_program, particle_buffer_read_index,
										this->particle_read_buffer_binding);
			glShaderStorageBlockBinding(this->particle_compute_program, particle_buffer_write_index,
										this->particle_write_buffer_binding);
			glShaderStorageBlockBinding(this->particle_compute_program, particle_buffer_vector_field_index,
										this->particle_vectorfield_buffer_binding);

			glUseProgram(0);

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->vector_field_graphic_program);
			int uniform_buffer_vector_field_index =
				glGetUniformBlockIndex(this->vector_field_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->vector_field_graphic_program, uniform_buffer_vector_field_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align<size_t>(this->uniformBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			GLint minStorageMapBufferSize;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageMapBufferSize);

			/*	Compute number of particles and memory size required, aligned to hardware min alignment.	*/
			this->nrParticles = (size_t)(uniformStageBuffer.particleSetting.particleBox.x *
										 uniformStageBuffer.particleSetting.particleBox.y *
										 uniformStageBuffer.particleSetting.particleBox.z);
			this->ParticleMemorySize =
				Math::align<size_t>(this->nrParticles * sizeof(Particle), minStorageMapBufferSize);

			/*	Create buffer.	*/
			glGenBuffers(1, &this->particles.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->particles.vbo);
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
							  Random::normalizeRand<float>() * this->uniformStageBuffer.particleSetting.lifetime);
				particle_buffer[i].velocity = glm::vec4(
					Random::normalizeRand<float>() * 100.0f, Random::normalizeRand<float>() * 100.0f,
					Random::normalizeRand<float>() * 100.0f, 1.0f / (Random::normalizeRand<float>() * 100.0f));
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->particles.vao);
			glBindVertexArray(this->particles.vao);

			glBindBuffer(GL_ARRAY_BUFFER, this->particles.vbo);
			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
								  reinterpret_cast<void *>(sizeof(float) * 4));

			glBindVertexArray(0);

			/*	*/
			this->VectorFieldMemorySize = this->nrParticles * sizeof(VectorForce);
			this->VectorFieldMemorySize = Math::align<size_t>(this->VectorFieldMemorySize, minStorageMapBufferSize);

			glGenVertexArrays(1, &this->vectorField.vao);
			glBindVertexArray(this->vectorField.vao);

			glGenBuffers(1, &this->vectorField.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->vectorField.vbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->VectorFieldMemorySize, nullptr, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, this->vectorField.vbo);

			/*	Position.	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

			/*	Velocity.	*/
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
								  reinterpret_cast<void *>(sizeof(glm::vec3)));

			glBindVertexArray(0);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->vectorField.vbo);
			VectorForce *vec_field =
				static_cast<VectorForce *>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->VectorFieldMemorySize,
															GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
			/*	Setup particle.	*/
			for (size_t i = 0; i < this->nrParticles; i++) {
				vec_field[i].position = glm::vec3(fragcore::Math::PerlinNoise(i, i * 2), 0, 0);
				vec_field[i].force =
					glm::vec3(fragcore::Math::PerlinNoise(i, i * 2), fragcore::Math::PerlinNoise(i, i * 4),
							  fragcore::Math::PerlinNoise(i, i * 5));
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			fragcore::resetErrorFlag();
		}
		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	Compute particles in vector field.	*/
			if (this->vectorFieldSettingComponent->simulateParticles) {
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*	Bind Vector field.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_vectorfield_buffer_binding,
								  this->vectorField.vbo, 0, this->VectorFieldMemorySize);

				/*	Bind read particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_read_buffer_binding, this->particles.vbo,
								  ((this->getFrameCount()) % this->nrParticleBuffers) * this->ParticleMemorySize,
								  this->ParticleMemorySize);

				/*	Bind write particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_write_buffer_binding, this->particles.vbo,
								  ((this->getFrameCount() + 1) % this->nrParticleBuffers) * this->ParticleMemorySize,
								  this->ParticleMemorySize);

				/*	Compute.	*/
				glUseProgram(this->particle_compute_program);
				glDispatchCompute(this->uniformStageBuffer.particleSetting.particleBox.x / this->localWorkGroupSize[0],
								  this->uniformStageBuffer.particleSetting.particleBox.y / this->localWorkGroupSize[1],
								  this->uniformStageBuffer.particleSetting.particleBox.z / this->localWorkGroupSize[2]);

				glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

				glUseProgram(0);
			}

			/*	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Draw Vector field.	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*  Draw grid.  */
				glDisable(GL_BLEND);
				glDisable(GL_CULL_FACE);

				glUseProgram(this->vector_field_graphic_program);

				glLineWidth(3.0f);

				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	Draw triangle.	*/
				glBindVertexArray(this->vectorField.vao);
				glDrawArrays(GL_POINTS, 0, this->nrParticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Draw Particles.	*/
			{

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, particle_texture);

				glUseProgram(this->particle_graphic_program);

				/*	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glDisable(GL_DEPTH_TEST);

				/*	*/
				glEnable(GL_PROGRAM_POINT_SIZE_ARB);
				glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

				/*	*/
				glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
				glPointParameterf(GL_POINT_SIZE_MIN_ARB, 1.0f);
				glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE_ARB, 1.0f);

				/*	Draw triangle.	*/
				glBindVertexArray(this->particles.vao);
				/*	Bind.	*/
				glBindVertexBuffer(0, this->particles.vbo,
								   ((this->getFrameCount() + 0) % this->nrParticleBuffers) * this->ParticleMemorySize,
								   0);
				glBindVertexBuffer(1, this->particles.vbo,
								   ((this->getFrameCount() + 0) % this->nrParticleBuffers) * this->ParticleMemorySize,
								   sizeof(glm::vec4));
				glDrawArrays(GL_POINTS, 0, this->nrParticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				/*	*/
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			/*	Bind buffer and update region with new data.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class VectorFieldSample : public GLSample<VectorField> {
	  public:
		VectorFieldSample() : GLSample<VectorField>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Cloth Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::VectorFieldSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
