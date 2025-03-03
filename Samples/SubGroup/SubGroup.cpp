#include "Exception.hpp"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SubGroupArea : public GLSampleWindow {
	  public:
		SubGroupArea() {
			this->setTitle("SubGroup Area");
			this->subgroupSettingComponent = std::make_shared<SubGroupAreaSettingComponent>();
			this->addUIComponent(this->subgroupSettingComponent);
		}

		using UniformBuffer = struct alignas(16) uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;
		};

		using SubGroupAreaUniform = struct alignas(16) subgroup_area_uniform_buffer_block {
			/*	*/
			glm::mat4 view;
			glm::mat4 proj;
			/*	*/
			glm::vec4 clip_plane;
			glm::vec4 bound_box_center;
			glm::vec4 bound_box_extended;
			/*	*/
			uint nrFaces;
			uint padding0;
			uint padding1;
			uint padding2;
		};

		using ResultBuffer = struct alignas(16) uniform_buffer_result_t {
			float area;
			float averageNormal;
		};

		const std::string subGroupAreaComputeShaderPath = "Shaders/area/area.comp.spv";

		/*	Texture shaders paths.	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag.spv";

		SubGroupAreaUniform subgroupArea{};
		UniformBuffer uniform_stage_buffer{};
		/*	*/

		int localWorkGroupSize[3]{};

		/*	Uniform buffer.	*/

		int texture_program{};
		unsigned int uniform_buffer_binding = 0;

		unsigned int uniform_buffer = 0;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBuffer);

		unsigned int subgroup_area_program = 0;
		unsigned int subgroup_uniform_buffer_binding = 0;
		unsigned int uniform_vertices_binding = 2;
		unsigned int uniform_indices_binding = 1;
		unsigned int uniform_result_binding = 3;

		/*	*/
		unsigned int result_buffer = 0;
		size_t result_buffer_size = sizeof(ResultBuffer);

		/*	*/
		unsigned int subgroup_area_uniform_buffer = 0;
		size_t subgroup_area_uniform_buffer_size = sizeof(SubGroupAreaUniform);

		/*	*/
		MeshObject cubeGeometry;

		CameraController camera;

		class SubGroupAreaSettingComponent : public nekomimi::UIComponent {

		  public:
			SubGroupAreaSettingComponent() { this->setName("SubGroup Settings"); }
			void draw() override {}

		  private:
		};
		std::shared_ptr<SubGroupAreaSettingComponent> subgroupSettingComponent;

		void Release() override {
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteBuffers(1, &this->result_buffer);
		}

		void Initialize() override {

			int x = 0;
			glGetIntegerv(GL_SUBGROUP_SUPPORTED_FEATURES_KHR, &x);
			this->getLogger().info("Support Basic {0}", (x & GL_SUBGROUP_FEATURE_BASIC_BIT_KHR) > 0);
			this->getLogger().info("Support Vote {0}", (x & GL_SUBGROUP_FEATURE_VOTE_BIT_KHR) > 0);
			this->getLogger().info("Support Arithmetic {0}", (x & GL_SUBGROUP_FEATURE_ARITHMETIC_BIT_KHR) > 0);
			this->getLogger().info("Support Ballot {0}", (x & GL_SUBGROUP_FEATURE_BALLOT_BIT_KHR) > 0);
			this->getLogger().info("Support Shuffle {0}", (x & GL_SUBGROUP_FEATURE_SHUFFLE_BIT_KHR) > 0);
			this->getLogger().info("Support Shuffle Relative {0}",
								   (x & GL_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT_KHR) > 0);
			this->getLogger().info("Support Clustered {0}", (x & GL_SUBGROUP_FEATURE_CLUSTERED_BIT_KHR) > 0);
			this->getLogger().info("Support Quad {0}", (x & GL_SUBGROUP_FEATURE_QUAD_BIT_KHR) > 0);

			int size = 0;
			glGetIntegerv(GL_SUBGROUP_SIZE_KHR, &size);
			this->getLogger().info("SubGroup Size {0}", size);

			int supported = 0;
			glGetIntegerv(GL_SUBGROUP_SUPPORTED_STAGES_KHR, &supported);
			//			this->getLogger().info("SubGroup Size {0}",size);

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> subgroup_area_compute_binary =
					IOUtil::readFileData<uint32_t>(this->subGroupAreaComputeShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> texture_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> texture_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create compute pipeline.	*/
				this->subgroup_area_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &subgroup_area_compute_binary);

				this->texture_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
			}

			/*	Setup compute pipeline.	*/
			glUseProgram(this->subgroup_area_program);
			glGetProgramiv(this->subgroup_area_program, GL_COMPUTE_WORK_GROUP_SIZE, this->localWorkGroupSize);
			int indices_read_index =
				glGetProgramResourceIndex(this->subgroup_area_program, GL_SHADER_STORAGE_BLOCK, "IndicesBuffer");
			int vertices_read_index =
				glGetProgramResourceIndex(this->subgroup_area_program, GL_SHADER_STORAGE_BLOCK, "VertexBuffer");
			int result_index =
				glGetProgramResourceIndex(this->subgroup_area_program, GL_SHADER_STORAGE_BLOCK, "Result");

			/*	*/
			glShaderStorageBlockBinding(this->subgroup_area_program, indices_read_index, this->uniform_indices_binding);
			glShaderStorageBlockBinding(this->subgroup_area_program, vertices_read_index,
										this->uniform_vertices_binding);
			glShaderStorageBlockBinding(this->subgroup_area_program, result_index, this->uniform_result_binding);
			glUseProgram(0);

			/*	Align the uniform buffer size to hardware specific.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align<size_t>(this->uniformSize, (size_t)minMapBufferSize);
			this->result_buffer_size = Math::align<size_t>(this->result_buffer_size, (size_t)minMapBufferSize);
			this->subgroup_area_uniform_buffer_size =
				Math::align<size_t>(this->subgroup_area_uniform_buffer_size, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->subgroup_area_uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->subgroup_area_uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->subgroup_area_uniform_buffer_size, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->result_buffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->result_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->result_buffer_size, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			/*  */
			Common::loadCube(this->cubeGeometry, 1);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			/*	*/
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->result_buffer);
			ResultBuffer *resultBuffer = (ResultBuffer *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
																		  this->result_buffer_size, GL_MAP_READ_BIT);
			getLogger().info("Area {0}", resultBuffer->area);
			getLogger().info("Normal {0}", resultBuffer->averageNormal);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			/*	Bind subset of the uniform buffer, that the graphic pipeline will use.	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	Clear default framebuffer.	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	Activate texture graphic pipeline.	*/
				glUseProgram(this->texture_program);

				/*	Disable culling of faces, allows faces to be visible from both directions.	*/
				glDisable(GL_CULL_FACE);

				/*	Draw triangle*/
				glBindVertexArray(this->cubeGeometry.vao);
				glDrawElements(GL_TRIANGLES, this->cubeGeometry.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*  Compute Area */
			{
				glUseProgram(this->subgroup_area_program);

				glm::mat4 model = this->uniform_stage_buffer.model;
				glUniformMatrix4fv(glGetUniformLocation(this->subgroup_area_program, "transform.model"), 1, GL_FALSE,
								   &model[0][0]);

				/*	Bind subset of the uniform buffer, that the graphic pipeline will use.	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->subgroup_uniform_buffer_binding,
								  this->subgroup_area_uniform_buffer, 0, this->subgroup_area_uniform_buffer_size);

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_vertices_binding, this->cubeGeometry.vbo, 0,
								  this->cubeGeometry.stride * this->cubeGeometry.nrVertices);

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_indices_binding, this->cubeGeometry.ibo, 0,
								  4 * this->cubeGeometry.nrIndicesElements);

				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_result_binding, this->result_buffer, 0,
								  sizeof(ResultBuffer));

				const size_t work_group_X =
					std::ceil((float)this->cubeGeometry.nrIndicesElements / (float)this->localWorkGroupSize[0]);

				glDispatchCompute(work_group_X, 1, 1);

				/*	Wait in till image has been written.	*/
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

				glUseProgram(0);
			}
		}

		void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniform stage buffer values.	*/
			this->uniform_stage_buffer.model = glm::mat4(1);
			this->uniform_stage_buffer.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.modelViewProjection =
				(this->uniform_stage_buffer.proj * this->camera.getViewMatrix());

			this->subgroupArea.nrFaces = this->cubeGeometry.nrIndicesElements / 3;
			this->subgroupArea.proj = this->uniform_stage_buffer.proj;
			this->subgroupArea.view = this->camera.getViewMatrix();

			/*	Update uniform buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformMappedMemory = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformMappedMemory, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);

			/*	Update uniform buffer.	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->subgroup_area_uniform_buffer);
			SubGroupAreaUniform *suggroup = (SubGroupAreaUniform *)glMapBufferRange(
				GL_UNIFORM_BUFFER, 0, this->subgroup_area_uniform_buffer_size, GL_MAP_WRITE_BIT);
			*suggroup = this->subgroupArea;
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SubGroupAreaGLSample : public GLSample<SubGroupArea> {
	  public:
		SubGroupAreaGLSample() : GLSample<SubGroupArea>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {

	const std::vector<const char *> required_extensions = {"GL_KHR_shader_subgroup"};

	try {
		glsample::SubGroupAreaGLSample sample;
		sample.run(argc, argv, required_extensions);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}