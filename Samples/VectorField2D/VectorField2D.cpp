#include "GLSampleWindow.h"
#include "Math/NormalDistribution.h"
#include "ShaderLoader.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <Importer/ImageImport.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class VectorField2D : public GLSampleWindow {
	  public:
		VectorField2D() : GLSampleWindow() {
			this->setTitle("VectorField2D");
			/*	*/
			this->vectorFieldSettingComponent =
				std::make_shared<ParticleSystemSettingComponent>(this->uniformStageBuffer, this->nrParticles);
			this->addUIComponent(this->vectorFieldSettingComponent);
		}

		const size_t particle_multiple_count = 8;
		/*	*/
		size_t nrParticles = 0;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = 0;

		/*	*/
		MeshObject particles;

		/*	*/
		unsigned int particle_texture;
		int localWorkGroupSize[3];

		/*	Shader pipeline programs.	*/
		unsigned int particle_graphic_program;
		unsigned int particle_compute_program;
		unsigned int particle_init_compute_program;
		unsigned int particle_motion_force_compute_program;
		unsigned int vector_field_graphic_program;

		using Motion = struct motion_t {
			glm::vec2 normalizedPos; /*  Position in pixel space.    */
			glm::vec2 velocity;		 /*  direction and magnitude of mouse movement.  */
			float radius = 10.0f;	 /*  Radius of incluense, also the pressure of input.    */
			float amplitude = 1.0;
			float noise = 0;
			float pad2 = 0;
		};

		using ParticleSetting = struct particle_setting_t {
			glm::uvec4 particleBox = glm::uvec4(256, 256, 1, 0);
			glm::uvec4 vectorfieldbox = glm::uvec4(32, 32, 32, 0); // Dummy

			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;

			float density = 1.0f;
			uint32_t nrparticles{};
			float spriteSize = 0.25f;
			float dragMag = 0.1f;
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};
			glm::vec4 color = glm::vec4(1, 0.1, 0.1, 1);

			/*	*/
			ParticleSetting particleSetting;
			Motion motion;

			/*	*/
			float delta{};

		} uniformStageBuffer;

		using Particle = struct particle_t {
			glm::vec4 position; /*	Position, time	*/
			glm::vec4 velocity; /*	Velocity, mass	*/
		};

		using VectorForce = struct vector_force_t {
			glm::vec3 position; /*	*/
			glm::vec3 force;	/*	*/
		};

		CameraController camera;

		/*	*/
		int particle_read_buffer_binding = 1;
		int particle_write_buffer_binding = 2;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		class ParticleSystemSettingComponent : public nekomimi::UIComponent {

		  public:
			ParticleSystemSettingComponent(struct uniform_buffer_block &uniform, const size_t &nrParticleInternal)
				: nrParticle(nrParticleInternal), uniform(uniform) {
				this->setName("Particle Settings");
			}
			void draw() override {
				ImGui::DragFloat("Speed", &this->uniform.particleSetting.speed, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Strength", &this->uniform.particleSetting.strength, 1, 0.0f, 100.0f);
				ImGui::ColorEdit4("Diffuse Color", &this->uniform.color[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Sprite Size", &this->uniform.particleSetting.spriteSize, 1, 0.0f, 10.0f);
				ImGui::DragFloat("Drag Global", &this->uniform.particleSetting.dragMag, 1, 0.0f, 10.0f);
				ImGui::TextUnformatted("Motion Input");
				ImGui::DragFloat("Radius Influence", &this->uniform.motion.radius, 1, -128.0f, 128.0f);
				ImGui::DragFloat("Amplitude Influence", &this->uniform.motion.amplitude, 1, 0, 128.0f);

				ImGui::DragFloat("Noise", &this->uniform.motion.noise, 1, 0, 16.0f);

				ImGui::Checkbox("Simulate Particles", &this->simulateParticles);
				ImGui::DragInt("Number Particles", (int *)&this->uniform.particleSetting.nrparticles, 1, 0,
							   this->nrParticle);
				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Draw VectorField2D", &this->drawVelocity);

				if (ImGui::Button("Reset Simulation")) {
					this->requestRest = true;
				}
			}

			bool simulateParticles = true;
			bool drawVelocity = false;
			bool showWireFrame = false;
			bool requestRest = false;
			const size_t &nrParticle;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<ParticleSystemSettingComponent> vectorFieldSettingComponent;

		/*	Particle.	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";

		const std::string particleInitComputeShaderPath = "Shaders/vectorfield/init_particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleComputeShaderPath = "Shaders/vectorfield/particle2D.comp.spv";
		/*	Particle Simulation in Vector Field.	*/
		const std::string particleMotionForceComputeShaderPath = "Shaders/vectorfield/apply_force_2D.comp.spv";

		/*	Motion vector graphic shader.	*/
		const std::string vectorFieldVertexShaderPath = "Shaders/vectorfield/vectorField.vert.spv";
		const std::string vectorFieldGeometryShaderPath = "Shaders/vectorfield/motion2D.geom.spv";
		const std::string vectorFieldFragmentPath = "Shaders/vectorfield/vectorField.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->particle_graphic_program);
			glDeleteProgram(this->particle_compute_program);
			glDeleteProgram(this->particle_motion_force_compute_program);
			glDeleteProgram(this->vector_field_graphic_program);

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
				std::vector<uint32_t> vertex_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->particleVertexShaderPath, this->getFileSystem());
				std::vector<uint32_t> geometry_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->particleGeometryShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->particleFragmentShaderPath, this->getFileSystem());

				/*	Load Graphic Program.	*/
				this->particle_graphic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary,
																				  &fragment_binary, &geometry_binary);

				/*	*/

				const std::vector<uint32_t> compute_particle_init_binary =
					IOUtil::readFileData<uint32_t>(this->particleInitComputeShaderPath, this->getFileSystem());
				const std::vector<uint32_t> compute_particle_binary =
					IOUtil::readFileData<uint32_t>(this->particleComputeShaderPath, this->getFileSystem());
				const std::vector<uint32_t> compute_motion_binary_binary =
					IOUtil::readFileData<uint32_t>(this->particleMotionForceComputeShaderPath, this->getFileSystem());

				/*	Load Compute.	*/
				this->particle_init_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_particle_init_binary);
				this->particle_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_particle_binary);
				this->particle_motion_force_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_motion_binary_binary);

				/*	Vector field.	*/
				vertex_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldVertexShaderPath, this->getFileSystem());
				geometry_binary = glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldGeometryShaderPath,
																		   this->getFileSystem());
				fragment_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->vectorFieldFragmentPath, this->getFileSystem());

				this->vector_field_graphic_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_binary, &fragment_binary, &geometry_binary);
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
			{
				glUseProgram(this->particle_compute_program);
				int uniform_buffer_particle_compute_index =
					glGetUniformBlockIndex(this->particle_compute_program, "UniformBufferBlock");
				int particle_buffer_read_index =
					glGetProgramResourceIndex(this->particle_compute_program, GL_SHADER_STORAGE_BLOCK, "ReadBuffer");
				int particle_buffer_write_index =
					glGetProgramResourceIndex(this->particle_compute_program, GL_SHADER_STORAGE_BLOCK, "WriteBuffer");
				glGetProgramiv(this->particle_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);

				/*	*/
				glUniformBlockBinding(this->particle_compute_program, uniform_buffer_particle_compute_index,
									  this->uniform_buffer_binding);
				/*	*/
				glShaderStorageBlockBinding(this->particle_compute_program, particle_buffer_read_index,
											this->particle_read_buffer_binding);
				glShaderStorageBlockBinding(this->particle_compute_program, particle_buffer_write_index,
											this->particle_write_buffer_binding);

				glUseProgram(0);
			}

			{
				glUseProgram(this->particle_motion_force_compute_program);
				int uniform_buffer_particle_compute_index =
					glGetUniformBlockIndex(this->particle_motion_force_compute_program, "UniformBufferBlock");
				glUniformBlockBinding(this->particle_motion_force_compute_program,
									  uniform_buffer_particle_compute_index, this->uniform_buffer_binding);

				int particle_buffer_read_index = glGetProgramResourceIndex(this->particle_motion_force_compute_program,
																		   GL_SHADER_STORAGE_BLOCK, "ReadBuffer");
				int particle_buffer_write_index = glGetProgramResourceIndex(this->particle_motion_force_compute_program,
																			GL_SHADER_STORAGE_BLOCK, "WriteBuffer");

				/*	*/
				glShaderStorageBlockBinding(this->particle_motion_force_compute_program, particle_buffer_read_index,
											this->particle_read_buffer_binding);
				glShaderStorageBlockBinding(this->particle_motion_force_compute_program, particle_buffer_write_index,
											this->particle_write_buffer_binding);

				glUseProgram(0);
			}

			{
				glUseProgram(this->particle_init_compute_program);
				int uniform_buffer_particle_compute_index =
					glGetUniformBlockIndex(this->particle_init_compute_program, "UniformBufferBlock");
				glUniformBlockBinding(this->particle_init_compute_program, uniform_buffer_particle_compute_index,
									  this->uniform_buffer_binding);

				int particle_buffer_read_index = glGetProgramResourceIndex(this->particle_init_compute_program,
																		   GL_SHADER_STORAGE_BLOCK, "ReadBuffer");
				int particle_buffer_write_index = glGetProgramResourceIndex(this->particle_init_compute_program,
																			GL_SHADER_STORAGE_BLOCK, "WriteBuffer");

				/*	*/
				glShaderStorageBlockBinding(this->particle_init_compute_program, particle_buffer_read_index,
											this->particle_read_buffer_binding);
				glShaderStorageBlockBinding(this->particle_init_compute_program, particle_buffer_write_index,
											this->particle_write_buffer_binding);

				glUseProgram(0);
			}

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->vector_field_graphic_program);
			int uniform_buffer_vector_field_index =
				glGetUniformBlockIndex(this->vector_field_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->vector_field_graphic_program, uniform_buffer_vector_field_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->uniformAlignBufferSize, minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniformStageBuffer), &uniformStageBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			GLint minStorageMapBufferSize = 0;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageMapBufferSize);

			/*	Compute number of particles and memory size required, aligned to hardware min alignment.	*/
			this->nrParticles =
				Math::product(&uniformStageBuffer.particleSetting.particleBox[0], 3) * particle_multiple_count;
			this->ParticleMemorySize = this->nrParticles * sizeof(Particle);
			this->ParticleMemorySize = Math::align<size_t>(this->ParticleMemorySize, minStorageMapBufferSize);

			/*	Create buffer.	*/
			glGenBuffers(1, &this->particles.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->particles.vbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->ParticleMemorySize * this->nrParticleBuffers, nullptr,
						 GL_DYNAMIC_DRAW);

			/*	*/
			Particle *particle_buffer = static_cast<Particle *>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->ParticleMemorySize, GL_MAP_WRITE_BIT));

			std::uniform_real_distribution<float> randomDirection(-1.0, 1.0); /* random floats between [-1.0, 1.0]	*/
			std::uniform_real_distribution<float> randomPosition(0.00001,
																 0.99999); /* random floats between [-1.0, 1.0] */
			RandomUniform<float> uniformRandom(-1, 0);
			std::default_random_engine generator;

			const float xSize = this->uniformStageBuffer.particleSetting.particleBox.x;
			const float ySize = this->uniformStageBuffer.particleSetting.particleBox.y;

			for (size_t i = 0; i < this->nrParticles; i++) {

				particle_buffer[i].position =
					glm::vec4(randomPosition(generator) * xSize, randomPosition(generator) * ySize, 0, 0);

				particle_buffer[i].velocity =
					glm::vec4(randomDirection(generator) * 1.0f, randomDirection(generator) * 1.0f,
							  randomDirection(generator) * 1.0f, 1.0f / (randomDirection(generator) * 1.0f));
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
								  reinterpret_cast<void *>(sizeof(glm::vec4)));

			glBindVertexArray(0);

			this->uniformStageBuffer.particleSetting.nrparticles = this->nrParticles;

			fragcore::resetErrorFlag();
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			const size_t read_buffer_index = (this->getFrameCount() + 1) % this->nrParticleBuffers;
			const size_t write_buffer_index = (this->getFrameCount() + 0) % this->nrParticleBuffers;

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			if (this->vectorFieldSettingComponent->requestRest) {

				// TODO: for each particle buffer.
				const uint nrWorkGroupsX = (std::ceil((float)(this->nrParticles / this->particle_multiple_count) /
													  (float)this->localWorkGroupSize[0]));

				/*	Compute.	*/
				glUseProgram(this->particle_init_compute_program);

				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	Bind read particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_read_buffer_binding, this->particles.vbo,
								  read_buffer_index * this->ParticleMemorySize, this->ParticleMemorySize);

				/*	Bind write particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_write_buffer_binding, this->particles.vbo,
								  write_buffer_index * this->ParticleMemorySize, this->ParticleMemorySize);

				glDispatchCompute(nrWorkGroupsX, 1, 1);

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				glUseProgram(0);

				this->vectorFieldSettingComponent->requestRest = false;
			}

			/*	Compute particles in vector field.	*/
			if (this->vectorFieldSettingComponent->simulateParticles &&
				this->uniformStageBuffer.particleSetting.speed > 0) {

				const uint nrWorkGroupsX = (std::ceil((float)(this->nrParticles / this->particle_multiple_count) /
													  (float)this->localWorkGroupSize[0]));

				/*	Compute.	*/
				glUseProgram(this->particle_compute_program);

				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	Bind read particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_read_buffer_binding, this->particles.vbo,
								  read_buffer_index * this->ParticleMemorySize, this->ParticleMemorySize);

				/*	Bind write particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_write_buffer_binding, this->particles.vbo,
								  write_buffer_index * this->ParticleMemorySize, this->ParticleMemorySize);

				glDispatchCompute(nrWorkGroupsX, 1, 1);

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				glUseProgram(0);

				/*	Check if mouse pressed down.	*/
				if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON) &&
					!this->getInput().getKeyPressed(SDL_SCANCODE_LALT)) {

					glUseProgram(this->particle_motion_force_compute_program);
					/*	Bind uniform buffer.	*/
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
									  this->uniformAlignBufferSize);

					/*	Bind read particle buffer.	*/
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_read_buffer_binding, this->particles.vbo,
									  read_buffer_index * this->ParticleMemorySize, this->ParticleMemorySize);

					/*	Bind write particle buffer.	*/
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_write_buffer_binding,
									  this->particles.vbo, write_buffer_index * this->ParticleMemorySize,
									  this->ParticleMemorySize);

					glDispatchCompute(nrWorkGroupsX, 1, 1);

					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					glUseProgram(0);
				}
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
			}

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Draw Vector field.	*/
			if (this->vectorFieldSettingComponent->drawVelocity &&
				this->uniformStageBuffer.particleSetting.nrparticles > 0) {

				glDisable(GL_BLEND);
				glDisable(GL_CULL_FACE);

				glUseProgram(this->vector_field_graphic_program);

				glLineWidth(1.0f);

				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindVertexArray(this->particles.vao);

				glDrawArrays(GL_POINTS, 0, this->uniformStageBuffer.particleSetting.nrparticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->vectorFieldSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	Draw Particles.	*/
			if (this->uniformStageBuffer.particleSetting.nrparticles > 0 &&
				this->uniformStageBuffer.particleSetting.spriteSize > 0) {

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, this->particle_texture);

				glUseProgram(this->particle_graphic_program);

				glDisable(GL_BLEND);
				glDisable(GL_CULL_FACE);
				/*	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glDisable(GL_DEPTH_TEST);

				/*	Draw triangle.	*/
				glBindVertexArray(this->particles.vao);

				glDrawArrays(GL_POINTS, 0, this->uniformStageBuffer.particleSetting.nrparticles);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				const float xHalf = this->uniformStageBuffer.particleSetting.particleBox.x / 2.f;
				const float yHalf = this->uniformStageBuffer.particleSetting.particleBox.y / 2.f;
				glm::mat4 proj = glm::ortho(-xHalf, xHalf, -yHalf, yHalf, -10.0f, 10.0f);

				glm::mat4 viewMatrix = glm::translate(glm::vec3(-xHalf, -yHalf, 0));
				/*	*/
				this->uniformStageBuffer.proj = proj;

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = viewMatrix;
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

				if (this->getInput().getMousePressed(Input::MouseButton::LEFT_BUTTON)) {
					int x = 0, y = 0;
					this->getInput().getMousePosition(&x, &y);
					this->uniformStageBuffer.motion.normalizedPos =
						glm::vec2(1, 1) - (glm::vec2(x, y) / glm::vec2(this->width(), this->height()));
					this->uniformStageBuffer.motion.normalizedPos.x =
						1.0f - this->uniformStageBuffer.motion.normalizedPos.x;
				}
			}

			/*	Bind buffer and update region with new data.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class VectorFieldSample : public GLSample<VectorField2D> {
	  public:
		VectorFieldSample() : GLSample<VectorField2D>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Particle Texture Path",
					cxxopts::value<std::string>()->default_value("asset/particle-cell.png"));
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
