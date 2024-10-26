#include "SampleHelper.h"
#include "Skybox.h"
#include <DataStructure/QuadTree.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class MarchingCube : public GLSampleWindow {
	  public:
		MarchingCube() : GLSampleWindow() {
			this->setTitle("MarchingCube");

			this->marchingCubeSettingComponent =
				std::make_shared<MarchingCubeSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->marchingCubeSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		/*	*/
		typedef struct chunk_t {
			MeshObject *marchingCube = nullptr;
			glm::vec3 position = glm::vec3(0);
		} Chunk;

		MeshObject marchingCube;
		std::vector<Chunk> chunks;
		fragcore::QuadTree<float> quadTree;

		int localWorkGroupSize[3];

		/*	Shader pipeline programs.	*/
		unsigned int marching_cube_graphic_program;
		unsigned int marching_cube_generate_compute_program;

		typedef struct _marching_cube_cell_data_t {
			glm::vec3 pos;
			float scale;
			glm::vec3 normal;
			float size;
		} MarchingCubeCellData;

		typedef struct marching_cube_settings_t {
			float voxel_size = 1;
			float threshold = 0.01f;
			float mag = 0.13f;
			float scale = 0.009f;
			glm::vec4 position_offset = glm::vec4(0);
			glm::vec4 random_offset = glm::vec4(0);
		} MarchingCubeSettings;

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Fog settings.	*/
			FogSettings fogSettings;

			MarchingCubeSettings settings; // TODO: relocate or something

		} uniformStageBuffer;

		CameraController camera;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int vertex_dat_buffer_binding = 1;

		unsigned int irradiance_texture;

		/*	*/
		unsigned int uniform_buffer;

		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t marchingCubeSize = 0;
		size_t marchingTotalCubeSize = 0;
		const size_t maxWorldChunkSize[3] = {16 * 2, 8, 16 * 2};

		const size_t max_points_per_voxel = 15; /*	*/

		class MarchingCubeSettingComponent : public nekomimi::UIComponent {
		  public:
			MarchingCubeSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("MarchingCube Settings");
			}

			void draw() override {

				/*	*/
				ImGui::TextUnformatted("Fog Settings");
				ImGui::DragInt("Fog Type", (int *)&this->uniform.fogSettings.fogType);
				ImGui::ColorEdit4("Fog Color", &this->uniform.fogSettings.fogColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Fog Density", &this->uniform.fogSettings.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->uniform.fogSettings.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->uniform.fogSettings.fogStart);
				ImGui::DragFloat("Fog End", &this->uniform.fogSettings.fogEnd);

				ImGui::TextUnformatted("Marching Cube Settings");
				if (ImGui::DragFloat3("Position Offset", &this->uniform.settings.position_offset[0])) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat3("Random Offset", &this->uniform.settings.random_offset[0])) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("Thread", &this->uniform.settings.threshold)) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("VoxelSize", &this->uniform.settings.voxel_size)) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("Mag", &this->uniform.settings.mag)) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("Scale", &this->uniform.settings.scale)) {
					this->needUpdate = true;
				}

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			int workgroupSize[3] = {2, 2, 2};
			bool needUpdate = true;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<MarchingCubeSettingComponent> marchingCubeSettingComponent;

		/*	Texture shaders paths.	*/
		const std::string vertexShaderPath = "Shaders/marchingcube/geometry.vert.spv";
		const std::string fragmentShaderPath = "Shaders/marchingcube/geometry.frag.spv";
		const std::string geometryShaderPath = "Shaders/marchingcube/geometry.geom.spv";

		/*	Particle Simulation in Vector Field.	*/
		const std::string groupVisualComputeShaderPath = "Shaders/marchingcube/marchingcube.comp.spv";

		void Release() override {

			/*	*/
			glDeleteProgram(this->marching_cube_graphic_program);
			glDeleteProgram(this->marching_cube_generate_compute_program);
			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			/*  */
			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	*/
				const std::vector<uint32_t> vertex_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_binary =
					glsample::IOUtil::readFileData<uint32_t>(this->geometryShaderPath, this->getFileSystem());

				/*	Load Graphic Program.	*/
				this->marching_cube_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary, nullptr);
				/*	*/
				const std::vector<uint32_t> compute_marching_cube_generator_binary =
					IOUtil::readFileData<uint32_t>(this->groupVisualComputeShaderPath, this->getFileSystem());
				/*	Load Compute.	*/
				this->marching_cube_generate_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_marching_cube_generator_binary);
			}

			/*	Setup instance graphic pipeline.	*/
			glUseProgram(this->marching_cube_graphic_program);

			/*	*/
			int uniform_buffer_index =
				glGetUniformBlockIndex(this->marching_cube_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->marching_cube_graphic_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			/*	*/
			int marching_data_write_index =
				glGetProgramResourceIndex(this->marching_cube_graphic_program, GL_SHADER_STORAGE_BLOCK, "GeomBuffer");

			glShaderStorageBlockBinding(this->marching_cube_graphic_program, marching_data_write_index,
										this->vertex_dat_buffer_binding);

			glUseProgram(0);

			/*	*/
			{
				glUseProgram(this->marching_cube_generate_compute_program);
				const int uniform_compute_index =
					glGetUniformBlockIndex(this->marching_cube_generate_compute_program, "UniformBufferBlock");

				marching_data_write_index = glGetProgramResourceIndex(this->marching_cube_generate_compute_program,
																	  GL_SHADER_STORAGE_BLOCK, "GeomBuffer");
				/*	*/
				glUniformBlockBinding(this->marching_cube_generate_compute_program, uniform_compute_index,
									  this->uniform_buffer_binding);
				/*	*/
				glShaderStorageBlockBinding(this->marching_cube_generate_compute_program, marching_data_write_index,
											this->vertex_dat_buffer_binding);

				glGetProgramiv(this->marching_cube_generate_compute_program, GL_COMPUTE_WORK_GROUP_SIZE,
							   this->localWorkGroupSize);
				glUseProgram(0);
			}

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniformStageBuffer), &this->uniformStageBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			{

				/*	*/
				this->chunks.resize(fragcore::Math::product<size_t>(this->maxWorldChunkSize, 3));

				size_t chunk_index = 0;
				for (size_t x = 0; x < this->maxWorldChunkSize[0]; x++) {
					for (size_t y = 0; y < this->maxWorldChunkSize[1]; y++) {
						for (size_t z = 0; z < this->maxWorldChunkSize[2]; z++) {

							this->chunks[chunk_index].marchingCube = &this->marchingCube;
							this->chunks[chunk_index].position = glm::vec3(x, y, z);
							chunk_index++;
						}
					}
				}

				GLint SSBO_align_offset;
				glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &SSBO_align_offset);

				/*	*/
				const size_t marching_cube_chunk_num_vertices =
					this->max_points_per_voxel * (size_t)fragcore::Math::product<int>(this->localWorkGroupSize, 3);

				const size_t max_nr_chunks = (size_t)fragcore::Math::product<size_t>(this->maxWorldChunkSize, 3);

				this->marchingCubeSize = marching_cube_chunk_num_vertices * sizeof(MarchingCubeCellData);
				marchingTotalCubeSize =
					fragcore::Math::align<size_t>(this->marchingCubeSize * max_nr_chunks, SSBO_align_offset);

				glGenVertexArrays(1, &marchingCube.vao);
				glBindVertexArray(marchingCube.vao);
				glBindVertexArray(0);

				glGenBuffers(1, &this->marchingCube.vbo);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->marchingCube.vbo);
				glBufferData(GL_SHADER_STORAGE_BUFFER, this->marchingTotalCubeSize, nullptr, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

				this->marchingCube.nrVertices = marching_cube_chunk_num_vertices;
			}
			fragcore::resetErrorFlag();

			/*	load Skybox Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);

			ProcessData util(this->getFileSystem());
			util.computeIrradiance(skytexture, this->irradiance_texture, 256, 128);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	*/
			if (this->marchingCubeSettingComponent->needUpdate) {

				refreshWholeRoundRobinBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer, this->nrUniformBuffer,
											 &this->uniformStageBuffer, this->uniformAlignBufferSize);

				glUseProgram(this->marching_cube_generate_compute_program);

				for (size_t chunk_index = 0; chunk_index < this->chunks.size(); chunk_index++) {

					const Chunk &chunk = this->chunks[chunk_index];

					uniform_buffer_block *uniformPointer = (uniform_buffer_block *)glMapBufferRange(
						GL_UNIFORM_BUFFER,
						((this->getFrameCount()) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
						this->uniformAlignBufferSize, GL_MAP_WRITE_BIT);
					uniformPointer->settings.position_offset = glm::vec4(chunk.position, 0);
					glUnmapBuffer(GL_UNIFORM_BUFFER);

					/*	Wait in till the */
					glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT);

					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->vertex_dat_buffer_binding,
									  chunk.marchingCube->vbo, chunk_index * this->marchingCubeSize,
									  this->marchingCubeSize);

					glDispatchCompute(1, 1, 1);
				}

				glUseProgram(0);

				this->marchingCubeSettingComponent->needUpdate = false;

				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
								GL_UNIFORM_BARRIER_BIT);
			}

			/*	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Wait in till the */

			{
				glUseProgram(this->marching_cube_graphic_program);

				/*	*/
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	*/
				glDisable(GL_BLEND);

				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->marchingCubeSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glActiveTexture(GL_TEXTURE0 + 10);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				/*	Draw Items.	*/
				glBindVertexArray(this->marchingCube.vao);

				/*	TODO: draw only visable sections.	*/
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->vertex_dat_buffer_binding, this->marchingCube.vbo);
				glDrawArrays(GL_TRIANGLES, 0, this->marchingCube.nrVertices * this->chunks.size());

				// glMultiDrawArraysIndirect
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniforms.	*/
			{
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.modelView = this->uniformStageBuffer.view * this->uniformStageBuffer.model;
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			/*	Bind buffer and update region with new data.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);

			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT);

			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	/*	*/
	class MarchingCubeSample : public GLSample<MarchingCube> {
	  public:
		MarchingCubeSample() : GLSample<MarchingCube>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("S,skybox", "Skybox Texture File Path",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {

		glsample::MarchingCubeSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}