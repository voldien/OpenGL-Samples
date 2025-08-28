#include "GLSampleSession.h"
#include "SampleHelper.h"
#include <DataStructure/QuadTree.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <Scene/Scene.h>
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
			this->camera.setFar(4000.0f);
			this->camera.setPosition(glm::vec3(105.5f));
			this->camera.lookAt(glm::vec3(0.f));

			/*	*/
			this->uniformStageBuffer.fogSettings.fogColor = glm::vec4(0.2f, 0.325f, 0.48f, 1);
			this->uniformStageBuffer.fogSettings.fogIntensity = 2.0f;
		}

		using DomainChunk = struct domain_chunk_t {};

		/*	*/
		using Chunk = struct chunk_t {
			MeshObject *marchingCube = nullptr;
			glm::vec3 position = glm::vec3(0);
		};

		MeshObject marchingCube;
		std::array<MeshObject, 2> marchingCubeSortedMesh;
		unsigned int currentMarchingCubeMeshIndex = 0;

		std::vector<Chunk> chunks;
		fragcore::QuadTree<float> quadTree;

		/*	Shader pipeline programs.	*/
		unsigned int marching_cube_transform_program = 0;
		unsigned int marching_cube_graphic_program = 0;
		unsigned int marching_cube_generate_compute_program = 0;
		int localWorkGroupSize[3]{};

		using MarchingCubeCellData = struct _marching_cube_cell_data_t {
			glm::vec3 pos;
			float scale;
			glm::vec3 normal;
			float size;
		};

		using MarchingCubeSettings = struct marching_cube_settings_t {
			float voxel_size = 75;
			float threshold = 0.1f;
			float mag = 1.50f;
			float scale = 0.029f;
			glm::vec4 position_offset = glm::vec4(0);
			glm::vec4 random_offset = glm::vec4(0);
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};

			/*	Fog settings.	*/
			FogSettings fogSettings;

			MarchingCubeSettings settings; // TODO: relocate or something

		} uniformStageBuffer;

		CameraController camera;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int vertex_dat_buffer_binding = 1;

		unsigned int transform_feedback_written_query{};
		unsigned int irradiance_texture{};

		/*	*/
		unsigned int uniform_buffer{};

		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t marchingCubeSize = 0;
		size_t marchingTotalCubeSize = 0;
		const size_t maxWorldChunkSize[3] = {8, 8, 8};

		bool waitingForResult = false;

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
				ImGui::ColorEdit4("Fog Color", &this->uniform.fogSettings.fogColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
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
				if (ImGui::DragFloat("Thread", &this->uniform.settings.threshold, 1, 0, 0, "%.6f")) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("VoxelSize", &this->uniform.settings.voxel_size)) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("Mag", &this->uniform.settings.mag)) {
					this->needUpdate = true;
				}
				if (ImGui::DragFloat("Scale", &this->uniform.settings.scale, 1, 0, 0, "%.6f")) {
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

		/*	*/
		const std::string vertexMarchingCubeShaderPath = "Shaders/marchingcube/geometry.vert.spv";

		const std::string geometryMarchingCubeShaderPath = "Shaders/marchingcube/geometry.geom.spv";

		/*	*/
		const std::string vertexMarchingCubeGraphicShaderPath = "Shaders/marchingcube/marchingcube_graphic.vert.spv";
		const std::string fragmentMarchingCubeShaderPath = "Shaders/marchingcube/marchingcube_graphic.frag.spv";

		/*	*/
		const std::string groupVisualComputeShaderPath = "Shaders/marchingcube/marchingcube.comp.spv";

		void Release() override {

			/*	*/
			glDeleteProgram(this->marching_cube_transform_program);
			glDeleteProgram(this->marching_cube_generate_compute_program);
			glDeleteProgram(this->marching_cube_graphic_program);

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
					glsample::IOUtil::readFileData<uint32_t>(this->vertexMarchingCubeShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary = glsample::IOUtil::readFileData<uint32_t>(
					this->fragmentMarchingCubeShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_binary = glsample::IOUtil::readFileData<uint32_t>(
					this->geometryMarchingCubeShaderPath, this->getFileSystem());

				const std::vector<uint32_t> marchingCubeVertexBinary = glsample::IOUtil::readFileData<uint32_t>(
					this->vertexMarchingCubeGraphicShaderPath, this->getFileSystem());

				/*	Load Graphic Program.	*/
				this->marching_cube_transform_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, nullptr, &geometry_binary);

				/*	Load Graphic Program.	*/
				this->marching_cube_graphic_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &marchingCubeVertexBinary, &fragment_binary, nullptr);
				/*	*/
				const std::vector<uint32_t> compute_marching_cube_generator_binary =
					IOUtil::readFileData<uint32_t>(this->groupVisualComputeShaderPath, this->getFileSystem());
				/*	Load Compute.	*/
				this->marching_cube_generate_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_marching_cube_generator_binary);
			}

			{
				/*	Setup instance graphic pipeline.	*/
				glUseProgram(this->marching_cube_graphic_program);

				/*	*/
				int uniform_buffer_index =
					glGetUniformBlockIndex(this->marching_cube_graphic_program, "UniformBufferBlock");
				glUniformBlockBinding(this->marching_cube_graphic_program, uniform_buffer_index,
									  this->uniform_buffer_binding);

				glUseProgram(0);

				/*	Setup instance graphic pipeline.	*/
				glUseProgram(this->marching_cube_transform_program);

				/*	*/
				uniform_buffer_index =
					glGetUniformBlockIndex(this->marching_cube_transform_program, "UniformBufferBlock");
				glUniformBlockBinding(this->marching_cube_transform_program, uniform_buffer_index,
									  this->uniform_buffer_binding);
				/*	*/
				int marching_data_write_index = glGetProgramResourceIndex(this->marching_cube_transform_program,
																		  GL_SHADER_STORAGE_BLOCK, "GeomBuffer");

				glShaderStorageBlockBinding(this->marching_cube_transform_program, marching_data_write_index,
											this->vertex_dat_buffer_binding);

				std::array<const char *, 4> feedbackVaryings = {"out_vertex_worldspace", "out_coord", "out_color",
																"out_normal_worldspace"};
				glTransformFeedbackVaryings(marching_cube_transform_program, feedbackVaryings.size(),
											feedbackVaryings.data(), GL_INTERLEAVED_ATTRIBS);

				glUseProgram(0);
			}

			/*	*/
			{
				glUseProgram(this->marching_cube_generate_compute_program);
				const int uniform_compute_index =
					glGetUniformBlockIndex(this->marching_cube_generate_compute_program, "UniformBufferBlock");

				int marching_data_write_index = glGetProgramResourceIndex(this->marching_cube_generate_compute_program,
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

			glCreateQueries(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1, &this->transform_feedback_written_query);

			/*	Uniform buffer.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, minMapBufferSize);

			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniformStageBuffer), &this->uniformStageBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Chunk Data for the compute shaders.	*/
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

				GLint SSBO_align_offset = 0;
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

			/*	MarchingCube Mesh For Rendering.	*/
			for (size_t i = 0; i < marchingCubeSortedMesh.size(); i++) {

				glGenVertexArrays(1, &marchingCubeSortedMesh[i].vao);
				glBindVertexArray(marchingCubeSortedMesh[i].vao);

				glGenBuffers(1, &this->marchingCubeSortedMesh[i].vbo);
				glBindBuffer(GL_ARRAY_BUFFER, this->marchingCubeSortedMesh[i].vbo);
				glBufferData(GL_ARRAY_BUFFER, this->marchingTotalCubeSize, nullptr, GL_DYNAMIC_DRAW);

				/*	Vertex.	*/
				glEnableVertexAttribArrayARB(0);
				glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, 12 + 8 + 12 + 12, nullptr);
				/*	Vertex.	*/
				glEnableVertexAttribArrayARB(1);
				glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, 12 + 8 + 12 + 12, (const void *)12);
				/*	Vertex.	*/
				glEnableVertexAttribArrayARB(2);
				glVertexAttribPointerARB(2, 3, GL_FLOAT, GL_FALSE, 12 + 8 + 12 + 12, (const void *)(12 + 8));
				/*	Vertex.	*/
				glEnableVertexAttribArrayARB(3);
				glVertexAttribPointerARB(3, 3, GL_FLOAT, GL_FALSE, 12 + 8 + 12 + 12, (const void *)(12 + 8 + 12));

				glBindVertexArray(0);
			}

			/*	load Skybox Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);

			MiscProcessingUtil util(this->getFileSystem());
			util.computeDiffuseIrradiance(skytexture, this->irradiance_texture, 256, 128);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void updateChunks() {

			refreshWholeRoundRobinBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer, this->nrUniformBuffer,
										 &this->uniformStageBuffer, this->uniformAlignBufferSize);

			/*	*/
			uniform_buffer_block *uniformPointer = (uniform_buffer_block *)glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount()) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			const Chunk &chunk = this->chunks[0];
			uniformPointer->settings.position_offset = glm::vec4(chunk.position, 0);
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT);

			/*	Compute Marching Cube Cells.	*/
			glUseProgram(this->marching_cube_generate_compute_program);
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->vertex_dat_buffer_binding, chunk.marchingCube->vbo, 0,
							  this->marchingTotalCubeSize);

			glDispatchCompute(this->maxWorldChunkSize[0], this->maxWorldChunkSize[1], this->maxWorldChunkSize[2]);

			glUseProgram(0);

			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

			/*	Render and construct sorted mesh.	*/
			{
				int nextMesh = (currentMarchingCubeMeshIndex + 1) % this->marchingCubeSortedMesh.size();
				MeshObject &currentMarchingCubeSortedMesh = this->marchingCubeSortedMesh[nextMesh];

				// TODO:
				/*	Draw and save geometry, transform feeedback.	*/
				glEnable(GL_RASTERIZER_DISCARD);

				glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, currentMarchingCubeSortedMesh.vbo);

				glUseProgram(this->marching_cube_transform_program);

				/*	Draw Items.	*/
				glBindVertexArray(this->marchingCube.vao);

				/*	TODO: draw only visable sections.	*/
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, this->vertex_dat_buffer_binding, this->marchingCube.vbo);

				glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, this->transform_feedback_written_query);
				glBeginTransformFeedback(GL_TRIANGLES);

				const Chunk &chunk = this->chunks[0];

				glDrawArrays(GL_TRIANGLES, 0,
							 chunk.marchingCube->nrVertices * this->maxWorldChunkSize[0] * this->maxWorldChunkSize[1] *
								 this->maxWorldChunkSize[2]);

				glEndTransformFeedback();
				glBindVertexArray(0);
				glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

				glDisable(GL_RASTERIZER_DISCARD);

				this->waitingForResult = true;
				this->marchingCubeSettingComponent->needUpdate = false;
			}
		}

		void draw() override {

			size_t width = 0, height = 0;
			this->getCurrentFrameBufferSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	*/
			if (this->marchingCubeSettingComponent->needUpdate) {
				this->updateChunks();
			}

			MeshObject &currentMarchingCubeSortedMesh = this->marchingCubeSortedMesh[currentMarchingCubeMeshIndex];
			this->getLogger().info(currentMarchingCubeSortedMesh.nrVertices);

			GLuint aviable = 0;
			glGetQueryObjectuiv(transform_feedback_written_query, GL_QUERY_RESULT_AVAILABLE, &aviable);
			if (aviable && this->waitingForResult) {
				GLuint primitives = 0;
				glGetQueryObjectuiv(transform_feedback_written_query, GL_QUERY_RESULT_NO_WAIT, &primitives);
				this->getLogger().info("Primitives Written {}", primitives);

				currentMarchingCubeSortedMesh.nrVertices = static_cast<size_t>(primitives * 3);
				currentMarchingCubeMeshIndex = (currentMarchingCubeMeshIndex + 1) % this->marchingCubeSortedMesh.size();
				this->waitingForResult = false;
			}

			/*	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Wait in till the */
			if (currentMarchingCubeSortedMesh.nrVertices > 0) {
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

				glActiveTexture(GL_TEXTURE0 + TextureTypeBinding::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				/*	Draw Items.	*/
				glBindVertexArray(currentMarchingCubeSortedMesh.vao);

				/*	TODO: draw only visable sections.	*/
				glDrawArrays(GL_TRIANGLES, 0, currentMarchingCubeSortedMesh.nrVertices);

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

	/*	Require, compute, transform feedback.	*/

	try {
		glsample::MarchingCubeSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}