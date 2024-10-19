#include "GLSampleWindow.h"
#include "Math/Math.h"
#include "RenderDesc.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <Importer/ImageImport.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ComputeGroupVisual : public GLSampleWindow {
	  public:
		ComputeGroupVisual() : GLSampleWindow() {
			this->setTitle("ComputeGroupVisual");

			this->computeGroupVisualSettingComponent =
				std::make_shared<ComputeGroupVisualSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->computeGroupVisualSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		/*	*/
		MeshObject sphereMesh;

		int localWorkGroupSize[3];

		/*	Shader pipeline programs.	*/
		unsigned int compute_visual_instance_graphic_program;
		unsigned int compute_group_visual_compute_program;

		typedef struct _instance_data_t {
			glm::mat4 model;
			glm::vec4 color;
		} InstanceData;

		struct uniform_buffer_block {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	*/
			unsigned int nrFaces;
			unsigned int nrElements;
			float delta;
			float scale = 0.3f;

		} uniformStageBuffer;

		CameraController camera;

		/*	*/
		unsigned int visual_indirect_buffer_binding = 3;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		/*	*/
		unsigned int uniform_buffer;
		unsigned int uniform_instance_buffer;
		unsigned int indirect_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceMemorySize = 0;
		const size_t maxGroupSize[3] = {24, 24, 24};

		class ComputeGroupVisualSettingComponent : public nekomimi::UIComponent {

		  public:
			ComputeGroupVisualSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Compute Group Visual Settings");
			}
			void draw() override {

				if (ImGui::InputInt3("WorkGroup", this->workgroupSize)) {
					needUpdate = true;
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
		std::shared_ptr<ComputeGroupVisualSettingComponent> computeGroupVisualSettingComponent;

		/*	Texture shaders paths.	*/
		const std::string vertexShaderPath = "Shaders/groupvisual/groupvisual_instance.vert.spv";
		const std::string fragmentShaderPath = "Shaders/groupvisual/groupvisual_instance.frag.spv";

		/*	Particle Simulation in Vector Field.	*/
		const std::string groupVisualComputeShaderPath = "Shaders/groupvisual/groupvisual.comp.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->compute_visual_instance_graphic_program);
			glDeleteProgram(this->compute_group_visual_compute_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteBuffers(1, &this->uniform_instance_buffer);
			glDeleteBuffers(1, &this->indirect_buffer);
		}

		void Initialize() override {

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

				/*	Load Graphic Program.	*/
				this->compute_visual_instance_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);

				/*	*/
				const std::vector<uint32_t> compute_visual_binary =
					IOUtil::readFileData<uint32_t>(this->groupVisualComputeShaderPath, this->getFileSystem());

				/*	Load Compute.	*/
				this->compute_group_visual_compute_program =
					ShaderLoader::loadComputeProgram(compilerOptions, &compute_visual_binary);
			}

			/*	Setup instance graphic pipeline.	*/
			glUseProgram(this->compute_visual_instance_graphic_program);
			glUniform1i(glGetUniformLocation(this->compute_visual_instance_graphic_program, "DiffuseTexture"), 0);
			/*	*/
			int uniform_buffer_index =
				glGetUniformBlockIndex(this->compute_visual_instance_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->compute_visual_instance_graphic_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			/*	*/
			const int uniform_instance_buffer_index = glGetProgramResourceIndex(
				this->compute_visual_instance_graphic_program, GL_SHADER_STORAGE_BLOCK, "UniformInstanceBlock");
			glShaderStorageBlockBinding(this->compute_visual_instance_graphic_program, uniform_instance_buffer_index,
										this->uniform_instance_buffer_binding);

			glUseProgram(0);

			{
				glUseProgram(this->compute_group_visual_compute_program);
				const int uniform_compute_index =
					glGetUniformBlockIndex(this->compute_group_visual_compute_program, "UniformBufferBlock");

				const int instance_data_write_index = glGetProgramResourceIndex(
					this->compute_group_visual_compute_program, GL_SHADER_STORAGE_BLOCK, "InstanceModel");

				const int indirect_buffer_index = glGetProgramResourceIndex(this->compute_group_visual_compute_program,
																			GL_SHADER_STORAGE_BLOCK, "IndirectWrite");

				/*	*/
				glUniformBlockBinding(this->compute_group_visual_compute_program, uniform_compute_index,
									  this->uniform_buffer_binding);
				/*	*/
				glShaderStorageBlockBinding(this->compute_group_visual_compute_program, instance_data_write_index,
											this->uniform_instance_buffer_binding);
				glShaderStorageBlockBinding(this->compute_group_visual_compute_program, indirect_buffer_index,
											this->visual_indirect_buffer_binding);

				glGetProgramiv(this->compute_group_visual_compute_program, GL_COMPUTE_WORK_GROUP_SIZE,
							   this->localWorkGroupSize);
				glUseProgram(0);
			}

			GLint maxWorkGroupCount[3];
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupCount[0]);
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupCount[1]);
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupCount[2]);

			maxWorkGroupCount[0] = Math::min<int>(maxGroupSize[0], maxWorkGroupCount[0]);
			maxWorkGroupCount[1] = Math::min<int>(maxGroupSize[1], maxWorkGroupCount[1]);
			maxWorkGroupCount[2] = Math::min<int>(maxGroupSize[2], maxWorkGroupCount[2]);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->uniformAlignBufferSize, minMapBufferSize);

			GLint SSBO_align_offset;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &SSBO_align_offset);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniformStageBuffer), &this->uniformStageBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenBuffers(1, &this->indirect_buffer);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->indirect_buffer);
			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsIndirectCommand) * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

			/*	*/
			const size_t maxInstances = (size_t)Math::product<int>(maxWorkGroupCount, 3) *
										(size_t)Math::product<int>(this->localWorkGroupSize, 3);
			this->uniformInstanceMemorySize =
				fragcore::Math::align(maxInstances * sizeof(InstanceData), (size_t)SSBO_align_offset);

			glGenBuffers(1, &this->uniform_instance_buffer);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, this->uniformInstanceMemorySize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

			/*  */
			Common::loadSphere(this->sphereMesh, 1, 8, 8);

			/*	*/
			this->uniformStageBuffer.nrFaces = this->sphereMesh.nrIndicesElements;
			this->uniformStageBuffer.nrElements =
				fragcore::Math::product(this->computeGroupVisualSettingComponent->workgroupSize, 3) *
				fragcore::Math::product<int>(localWorkGroupSize, 3);

			fragcore::resetErrorFlag();
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			if (this->computeGroupVisualSettingComponent->needUpdate) {

				refreshWholeRoundRobinBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer, this->nrUniformBuffer,
											 &this->uniformStageBuffer, this->uniformAlignBufferSize);

				glUseProgram(this->compute_group_visual_compute_program);

				/*	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding,
								  this->uniform_instance_buffer, 0, this->uniformInstanceMemorySize);
				/*	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->visual_indirect_buffer_binding, this->indirect_buffer,
								  0, sizeof(IndirectDrawElement));

				/*	*/
				const size_t Xinvoke = Math::clamp<size_t>(this->computeGroupVisualSettingComponent->workgroupSize[0],
														   1, this->maxGroupSize[0]);
				const size_t Yinvoke = Math::clamp<size_t>(this->computeGroupVisualSettingComponent->workgroupSize[1],
														   1, this->maxGroupSize[1]);
				const size_t Zinvoke = Math::clamp<size_t>(this->computeGroupVisualSettingComponent->workgroupSize[2],
														   1, this->maxGroupSize[2]);
				glDispatchCompute(Xinvoke, Yinvoke, Zinvoke);

				glUseProgram(0);

				this->computeGroupVisualSettingComponent->needUpdate = false;
			}

			/*	*/
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			/*	*/
			glViewport(0, 0, width, height);

			/*	Wait in till the */
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT |
							GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

			{

				glUseProgram(this->compute_visual_instance_graphic_program);

				/*	*/
				glBindBufferRange(GL_SHADER_STORAGE_BUFFER, this->uniform_instance_buffer_binding,
								  this->uniform_instance_buffer, 0, this->uniformInstanceMemorySize);

				/*	*/
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				glFrontFace(GL_CW);

				/*	*/
				glDisable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glEnable(GL_DEPTH_TEST);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK,
							  this->computeGroupVisualSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw Items.	*/
				glBindVertexArray(this->sphereMesh.vao);

				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->indirect_buffer);
				glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
				glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
				glBindVertexArray(0);

				glUseProgram(0);
			}
		}

		void update() override {

			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniforms.	*/
			{
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

				this->uniformStageBuffer.delta = this->getTimer().deltaTime<float>();
				// TODO: add clamping, within
				this->uniformStageBuffer.nrElements =
					fragcore::Math::product(this->computeGroupVisualSettingComponent->workgroupSize, 3) *
					fragcore::Math::product<int>(localWorkGroupSize, 3);

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
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
	class ComputeGroupVisualSample : public GLSample<ComputeGroupVisual> {
	  public:
		ComputeGroupVisualSample() : GLSample<ComputeGroupVisual>() {}
		void customOptions(cxxopts::OptionAdder &options) override {}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ComputeGroupVisualSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
