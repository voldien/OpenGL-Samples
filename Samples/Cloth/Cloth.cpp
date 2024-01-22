#include "GLSampleSession.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <Importer/ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ClothSimulation : public GLSampleWindow {
	  public:
		ClothSimulation() : GLSampleWindow() {
			this->setTitle("Cloth Simulation");

			/*	*/
			this->clothSettingComponent = std::make_shared<ClothSimulationSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->clothSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		/*	*/
		size_t nrParticles = 0;
		const size_t nrParticleBuffers = 3;
		size_t ParticleMemorySize = 0;
		size_t VectorFieldMemorySize = 0;

		/*	*/
		GeometryObject cloth;
		GeometryObject grid;

		/*	*/
		unsigned int particle_texture;
		unsigned int grid_texture;

		int localWorkGroupSize[3];

		/*	*/
		unsigned int cloth_graphic_program;
		unsigned int cloth_compute_program;

		typedef struct wind_settings_t {
			glm::vec4 direction;
			glm::vec4 noise;
		} WindSettings;

		typedef struct particle_setting_t {
			glm::uvec3 particleBox = glm::uvec3(16, 16, 16);
			float speed = 1.0f;
			float lifetime = 5.0f;
			float gravity = 9.82f;
			float strength = 1.0f;
			float density = 1.0f;

		} ClothSetting;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	*/
			ClothSetting clothSetting;
			WindSettings windSettings;

			struct light_settings_t {
				/*	light source.	*/
				glm::vec4 lookDirection;
				glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
				glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
				glm::vec4 position;
			} light;

			/*	*/
			float delta;

		} uniformBuffer;

		typedef struct particle_t {
			ProceduralGeometry::Vertex vertex; /*	Position, time	*/
			glm::vec4 velocity;				   /*	Velocity, mass	*/
			float pinnedWeight;
		} ClothVertex;

		CameraController camera;

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

		class ClothSimulationSettingComponent : public nekomimi::UIComponent {

		  public:
			ClothSimulationSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Cloth Simulation Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Light Setting");
				{
					ImGui::ColorEdit4("Color", &this->uniform.light.lightColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					ImGui::ColorEdit4("Ambient", &this->uniform.light.ambientLight[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					// ImGui::ColorEdit4("Specular Color", &this->uniform.ocean.specularColor[0],
					//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					//
					if (ImGui::DragFloat3("Light Direction", &this->uniform.light.lightDirection[0])) {
						this->uniform.light.lightDirection = glm::normalize(this->uniform.light.lightDirection);
					}
				}

				/*	Wind Settings.	*/
				ImGui::TextUnformatted("Wind Settings");
				{
					ImGui::DragFloat3("Direction", &this->uniform.windSettings.direction[0]);
					ImGui::DragFloat3("Noise", &this->uniform.windSettings.noise[0]);
				}
				ImGui::Checkbox("WireFrame", &this->selfCollision);

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool simulateCloth = false;
			bool showWireFrame = false;
			bool selfCollision = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<ClothSimulationSettingComponent> clothSettingComponent;

		/*	*/
		const std::string clothVertexShaderPath = "Shaders/cloth/cloth.vert.spv";
		const std::string clothFragmentShaderPath = "Shaders/cloth/cloth.frag.spv";

		/*	*/
		const std::string particleComputeShaderPath = "Shaders/cloth/cloth.comp.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->cloth_graphic_program);
			glDeleteProgram(this->cloth_compute_program);

			glDeleteTextures(1, &this->particle_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string texturePath = this->getResult()["texture"].as<std::string>();

			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	*/
				std::vector<char> vertex_source =
					glsample::IOUtil::readFileString(this->clothVertexShaderPath, this->getFileSystem());
				std::vector<char> fragment_source =
					glsample::IOUtil::readFileString(this->clothFragmentShaderPath, this->getFileSystem());

				/*	*/
				std::vector<char> compute_source =
					glsample::IOUtil::readFileString(this->particleComputeShaderPath, this->getFileSystem());

				/*	Load Graphic Program.	*/
				this->cloth_graphic_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);
				/*	Load Compute.	*/
				this->cloth_compute_program = ShaderLoader::loadComputeProgram({&compute_source});
			}

			/*	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->particle_texture = textureImporter.loadImage2D(texturePath);

			/*	Setup graphic render pipeline.	*/
			glUseProgram(this->cloth_graphic_program);
			glUniform1i(glGetUniformLocation(this->cloth_graphic_program, "spriteTexture"), 0);
			int uniform_buffer_particle_graphic_index =
				glGetUniformBlockIndex(this->cloth_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->cloth_graphic_program, uniform_buffer_particle_graphic_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup particle simulation pipeline.	*/
			glUseProgram(this->cloth_compute_program);
			int uniform_buffer_particle_compute_index =
				glGetUniformBlockIndex(this->cloth_compute_program, "UniformBufferBlock");
			this->particle_buffer_vector_field_index =
				glGetUniformBlockIndex(this->cloth_compute_program, "VectorField");
			this->particle_buffer_read_index = glGetUniformBlockIndex(this->cloth_compute_program, "ReadBuffer");
			this->particle_buffer_write_index = glGetUniformBlockIndex(this->cloth_compute_program, "WriteBuffer");
			glGetProgramiv(this->cloth_compute_program, GL_COMPUTE_WORK_GROUP_SIZE, localWorkGroupSize);
			// TODO fix
			this->particle_buffer_read_index = 1;
			this->particle_buffer_write_index = 2;

			glUniformBlockBinding(this->cloth_compute_program, uniform_buffer_particle_compute_index,
								  this->uniform_buffer_binding);
			/*	*/
			glShaderStorageBlockBinding(this->cloth_compute_program, this->particle_buffer_read_index,
										this->particle_read_buffer_binding);
			glShaderStorageBlockBinding(this->cloth_compute_program, this->particle_buffer_write_index,
										this->particle_write_buffer_binding);

			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align<size_t>(this->uniformBufferSize, minMapBufferSize);
			
			// GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices, 2048, 2048);
				std::vector<ClothVertex> ClothVertex(vertices.size());

				for (size_t i = 0; i < this->nrParticles * this->nrParticleBuffers; i++) {
				}

				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ClothVertex), nullptr);

				/*	UV.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ClothVertex), reinterpret_cast<void *>(12));

				/*	Normal.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ClothVertex), reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ClothVertex), reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}

			fragcore::resetErrorFlag();
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	Compute cloth.	*/
			if (this->clothSettingComponent->simulateCloth) {
				{
					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

					/*	Bind uniform buffer.	*/
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
									  this->uniformBufferSize);
					/*	*/
					// glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_vector_field_index,
					//				  this->vectorField.vbo, 0, this->VectorFieldMemorySize);

					///*	Bind read particle buffer.	*/
					// glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_read_index, this->vbo_particle,
					//				  ((this->getFrameCount()) % nrParticleBuffers) * ParticleMemorySize,
					//				  this->ParticleMemorySize);
					//
					///*	Bind write particle buffer.	*/
					// glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->particle_buffer_write_index,
					// this->vbo_particle,
					//				  ((this->getFrameCount() + 1) % this->nrParticleBuffers) *
					//					  this->ParticleMemorySize,
					//				  this->ParticleMemorySize);

					/*	Compute.	*/
					glUseProgram(this->cloth_compute_program);
					glDispatchCompute(cloth.nrVertices / this->localWorkGroupSize[0],
									  cloth.nrVertices / this->localWorkGroupSize[1], 1);

					glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

					glUseProgram(0);
				}
			}

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Cloth.	*/
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
								  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*	Draw Skybox.	*/

				glDisable(GL_CULL_FACE);

				/*	Draw terrain.	*/
				glUseProgram(this->cloth_graphic_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->particle_texture);

				glBindVertexArray(this->cloth.vao);
				glDrawElements(GL_TRIANGLES, cloth.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
		}

		void update() override {
			/*	*/
			this->uniformBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);

			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			this->uniformBuffer.delta = this->getTimer().deltaTime<float>();

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
	/*	*/
	class ClothSimulationSample : public GLSample<ClothSimulation> {
	  public:
		ClothSimulationSample() : GLSample<ClothSimulation>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Cloth Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ClothSimulationSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
