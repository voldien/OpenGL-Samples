#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderCompiler.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class CirclePacking : public GLSampleWindow {
	  public:
		CirclePacking() : GLSampleWindow() {
			this->setTitle("CirclePacking - Compute");
			this->vectorFieldSettingComponent =
				std::make_shared<CirclePackingSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->vectorFieldSettingComponent);
		}

		typedef struct circle_t {
			float position[3];
			float radius;
		} Circle;

		struct uniform_buffer_block {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;
			glm::vec4 color = glm::vec4(1);

			/*	*/

			/*	*/
			float delta;
			float speed;

		} uniformStageBuffer;

		/*	*/
		size_t nrCircles = 0;
		const size_t nrParticleBuffers = 2;
		size_t ParticleMemorySize = 0;

		/*	*/
		MeshObject circles_points;

		unsigned int particle_texture;

		/*	*/
		unsigned int particle_graphic_program;
		unsigned int circle_packing_program;
		int localWorkGroupSize[3];

		unsigned int nthTexture = 0;
		CameraController camera;
		/*	*/
		int circle_packing_read_buffer_binding = 1;
		int circle_packing_write_buffer_binding = 2;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		/*	Particle.	*/
		const std::string particleVertexShaderPath = "Shaders/vectorfield/particle.vert.spv";
		const std::string particleGeometryShaderPath = "Shaders/vectorfield/particle.geom.spv";
		const std::string particleFragmentShaderPath = "Shaders/vectorfield/particle.frag.spv";
		/*	*/
		const std::string computeShaderPath = "Shaders/circlepacking/circlepacking.comp.spv";

		class CirclePackingSettingComponent : public nekomimi::UIComponent {

		  public:
			CirclePackingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Particle Settings");
			}
			void draw() override {
				// ImGui::DragFloat("Speed", &this->uniform.particleSetting.speed, 1, 0.0f, 100.0f);
				// ImGui::DragFloat("Strength", &this->uniform.particleSetting.strength, 1, 0.0f, 100.0f);
				// ImGui::DragFloat("LifeTime", &this->uniform.particleSetting.lifetime, 1, 0.0f, 10.0f);
				// ImGui::ColorEdit4("Diffuse Color", &this->uniform.color[0],
				// 				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat("Sprite Size", &this->uniform.particleSetting.spriteSize, 1, 0.0f, 10.0f);

				// ImGui::DragFloat("Radius Influence", &this->uniform.motion.radius, 1, -128.0f, 128.0f);
				// ImGui::DragFloat("Amplitude Influence", &this->uniform.motion.amplitude, 1, 0, 128.0f);

				// ImGui::Checkbox("Simulate Particles", &this->simulateParticles);
				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Draw VectorField2D", &this->drawVectorField);
			}

			bool simulateParticles = true;
			bool drawVectorField = false;
			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<CirclePackingSettingComponent> vectorFieldSettingComponent;

		void Release() override {
			glDeleteProgram(this->circle_packing_program);

			// glDeleteTextures(this->gameoflife_state_texture.size(),
			//				 (const GLuint *)this->gameoflife_state_texture.data());
			// glDeleteTextures(1, &this->gameoflife_render_texture);
		}

		void Initialize() override {

			/*	*/
			const std::string particleTexturePath = this->getResult()["texture"].as<std::string>();

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> gameoflife_compute_binary =
					IOUtil::readFileData<uint32_t>(this->computeShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->circle_packing_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &gameoflife_compute_binary);
			}

			/*	Setup particle graphic render pipeline.	*/
			glUseProgram(this->particle_graphic_program);
			glUniform1i(glGetUniformLocation(this->particle_graphic_program, "spriteTexture"), 0);
			int uniform_buffer_particle_graphic_index =
				glGetUniformBlockIndex(this->particle_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->particle_graphic_program, uniform_buffer_particle_graphic_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup compute pipeline.	*/
			glUseProgram(this->circle_packing_program);
			int uniform_buffer_particle_compute_index =
				glGetUniformBlockIndex(this->circle_packing_program, "UniformBufferBlock");
			glUniformBlockBinding(this->circle_packing_program, uniform_buffer_particle_compute_index,
								  this->uniform_buffer_binding);

			int particle_buffer_read_index =
				glGetProgramResourceIndex(this->circle_packing_program, GL_SHADER_STORAGE_BLOCK, "CurrentPacking");
			int particle_buffer_write_index =
				glGetProgramResourceIndex(this->circle_packing_program, GL_SHADER_STORAGE_BLOCK, "PreviousPacking");

			/*	*/
			glShaderStorageBlockBinding(this->circle_packing_program, particle_buffer_read_index,
										this->circle_packing_read_buffer_binding);
			glShaderStorageBlockBinding(this->circle_packing_program, particle_buffer_write_index,
										this->circle_packing_write_buffer_binding);

			glGetProgramiv(this->circle_packing_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			glUseProgram(0);

			GLint minStorageMapBufferSize;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minStorageMapBufferSize);

			/*	Compute number of particles and memory size required, aligned to hardware min alignment.	*/
			this->nrCircles = 100;
			this->ParticleMemorySize = this->nrCircles * sizeof(Circle);
			this->ParticleMemorySize = Math::align<size_t>(this->ParticleMemorySize, minStorageMapBufferSize);

			/*	Create buffer.	*/
			glGenBuffers(1, &this->circles_points.vbo);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->circles_points.vbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->ParticleMemorySize * this->nrParticleBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->circles_points.vao);
			glBindVertexArray(this->circles_points.vao);

			glBindBuffer(GL_ARRAY_BUFFER, this->circles_points.vbo);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Circle), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Circle),
								  reinterpret_cast<void *>(sizeof(glm::vec3)));

			glBindVertexArray(0);

			fragcore::resetErrorFlag();
		}

		void onResize(int width, int height) override {}

		void draw() override {

			size_t read_buffer_index = (this->getFrameCount() + 1) % this->nrParticleBuffers;
			size_t write_buffer_index = (this->getFrameCount() + 0) % this->nrParticleBuffers;

			int width, height;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	Bind and Compute Game of Life Compute Program.	*/
			if (this->vectorFieldSettingComponent->simulateParticles && this->uniformStageBuffer.speed > 0) {

				const uint nrWorkGroupsX = std::ceil((float)this->nrCircles / (float)this->localWorkGroupSize[0]);

				glUseProgram(this->circle_packing_program);

				/*	Bind uniform buffer.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*	Bind read particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->circle_packing_read_buffer_binding,
								  this->circles_points.vbo, read_buffer_index * this->ParticleMemorySize,
								  this->ParticleMemorySize);

				/*	Bind write particle buffer.	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->circle_packing_write_buffer_binding,
								  this->circles_points.vbo, write_buffer_index * this->ParticleMemorySize,
								  this->ParticleMemorySize);

				glDispatchCompute(nrWorkGroupsX, 1, 1);

				glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
				glUseProgram(0);
			}

			/*	Blit game of life render framebuffer to default framebuffer.	*/
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Draw Particles.	*/
			{

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
				glBindVertexArray(this->circles_points.vao);
				// glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
				// 					  reinterpret_cast<void *>(read_buffer_index * this->ParticleMemorySize));
				// glVertexAttribPointer(
				// 	1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle),
				// 	reinterpret_cast<void *>(read_buffer_index * this->ParticleMemorySize + sizeof(glm::vec4)));
				glDrawArrays(GL_POINTS, 0, this->nrCircles);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				const float xHalf = 2; // this->uniformStageBuffer.particleSetting.particleBox.x / 2.f;
				const float yHalf = 2; // this->uniformStageBuffer.particleSetting.particleBox.y / 2.f;
				glm::mat4 proj = glm::ortho(-xHalf, xHalf, -yHalf, yHalf, -10.0f, 10.0f);

				glm::mat4 viewMatrix = glm::translate(glm::vec3(-xHalf, -yHalf, 0));
				/*	*/
				this->uniformStageBuffer.proj = proj;

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = viewMatrix;
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}
		}
	};

	/*	*/
	class CirclePackingSample : public GLSample<CirclePacking> {
	  public:
		CirclePackingSample() : GLSample<CirclePacking>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Particle Texture Path",
					cxxopts::value<std::string>()->default_value("asset/particle-cell.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::CirclePackingSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
