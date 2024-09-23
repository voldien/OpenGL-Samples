#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SkinnedMesh : public GLSampleWindow {

	  public:
		SkinnedMesh() : GLSampleWindow() {
			this->setTitle("Skinned Mesh");

			this->skinnedSettingComponent = std::make_shared<SkinnedMeshSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->skinnedSettingComponent);

			this->camera.setPosition(glm::vec3(18.5f));
			this->camera.lookAt(glm::vec3(0.f));
			this->camera.enableLook(true);
			this->camera.enableNavigation(true);
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(0.5f, 0.5f, 0.6f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.05, 0.05, 0.05, 1.0f);

			/*	Material color.	*/
			glm::vec4 tintColor = glm::vec4(1);

		} uniformStageBuffer;

		CameraController camera;

		Scene scene;

		unsigned int skinned_graphic_program;
		unsigned int skinned_debug_weight_program;
		unsigned int skinned_bone_program;
		unsigned int axis_orientation_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_skeleton_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_skeleton_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t uniformSkeletonBufferSize = 0;
		SkeletonSystem skeleton;

		/*	*/
		const std::string vertexSkinnedShaderPath = "Shaders/skinnedmesh/skinnedmesh.vert.spv";
		const std::string fragmentSkinnedShaderPath = "Shaders/skinnedmesh/skinnedmesh.frag.spv";

		const std::string vertexSkinnedBoneShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.vert.spv";
		const std::string fragmentSkinnedBoneShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.frag.spv";

		const std::string vertexSkinnedDebugShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.vert.spv";
		const std::string fragmentSkinnedDebugShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.frag.spv";

		const std::string vertexAxisShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.vert.spv";
		const std::string fragmentAxisShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.frag.spv";

		class SkinnedMeshSettingComponent : public nekomimi::UIComponent {

		  public:
			SkinnedMeshSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Skinned Mesh");
			}
			void draw() override {
				ImGui::TextUnformatted("Light Settings");

				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Debugging");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Show Bone", &this->showBone);
				ImGui::Checkbox("Show Weight", &this->showWeight);
				ImGui::Checkbox("Show Axis", &this->showAxis);
			}

			bool showWireFrame = false;
			bool showBone = false;
			bool showWeight = false;
			bool showAxis = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<SkinnedMeshSettingComponent> skinnedSettingComponent;

		void Release() override {
			/*	*/
			glDeleteProgram(this->skinned_graphic_program);
			glDeleteProgram(this->skinned_debug_weight_program);
			glDeleteProgram(this->skinned_bone_program);
			glDeleteProgram(this->axis_orientation_program);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Main skinned shaders.	*/
				const std::vector<uint32_t> skinned_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkinnedShaderPath, this->getFileSystem());
				const std::vector<uint32_t> skinned_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkinnedShaderPath, this->getFileSystem());

				/*	Skinned bone debug */
				const std::vector<uint32_t> skinned_debug_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkinnedDebugShaderPath, this->getFileSystem());
				const std::vector<uint32_t> skinned_debug_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkinnedDebugShaderPath, this->getFileSystem());

				/*	Skinned bone weight.	*/
				const std::vector<uint32_t> skinned_bone_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkinnedBoneShaderPath, this->getFileSystem());
				const std::vector<uint32_t> skinned_bone_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkinnedBoneShaderPath, this->getFileSystem());

				/*	Bone Axis	*/
				const std::vector<uint32_t> axis_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexAxisShaderPath, this->getFileSystem());
				const std::vector<uint32_t> axis_bone_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentAxisShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->skinned_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &skinned_vertex_binary, &skinned_fragment_binary);

				/*	Load shader	*/
				this->skinned_debug_weight_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &skinned_debug_vertex_binary, &skinned_debug_fragment_binary);

				/*	Load shader	*/
				this->skinned_bone_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &skinned_bone_vertex_binary, &skinned_bone_fragment_binary);

				/*	Load shader	*/
				this->axis_orientation_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &axis_vertex_binary, &axis_bone_fragment_binary);
			}

			/*	*/
			glUseProgram(this->skinned_graphic_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->skinned_graphic_program, "UniformBufferBlock");
			int uniform_skeleton_buffer_index =
				glGetUniformBlockIndex(this->skinned_graphic_program, "UniformSkeletonBufferBlock");
			glUniform1i(glGetUniformLocation(this->skinned_graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->skinned_graphic_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->skinned_graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUniformBlockBinding(this->skinned_graphic_program, uniform_skeleton_buffer_index,
								  this->uniform_skeleton_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skinned_debug_weight_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skinned_debug_weight_program, "UniformBufferBlock");
			uniform_skeleton_buffer_index =
				glGetUniformBlockIndex(this->skinned_debug_weight_program, "UniformSkeletonBufferBlock");
			glUniformBlockBinding(this->skinned_debug_weight_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUniformBlockBinding(this->skinned_debug_weight_program, uniform_skeleton_buffer_index,
								  this->uniform_skeleton_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skinned_bone_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skinned_bone_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skinned_bone_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->axis_orientation_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->axis_orientation_program, "UniformBufferBlock");
			glUniformBlockBinding(this->axis_orientation_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Create all uniform buffers.	*/
			{
				/*	Align uniform buffer in respect to driver requirement.	*/
				GLint minMapBufferSize;
				glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
				this->uniformAlignBufferSize = fragcore::Math::align(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

				/*	Create uniform buffer.	*/
				glGenBuffers(1, &this->uniform_buffer);
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);

				/*	*/
				GLint uniformMaxSize;
				glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniformMaxSize);
				int instanceBatch = uniformMaxSize / sizeof(glm::mat4);

				this->uniformSkeletonBufferSize =
					fragcore::Math::align(instanceBatch * sizeof(glm::mat4), (size_t)minMapBufferSize);

				// Create skeleton buffer.
				glGenBuffers(1, &this->uniform_skeleton_buffer);
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_skeleton_buffer);
				glBufferData(GL_UNIFORM_BUFFER, this->uniformSkeletonBufferSize * this->nrUniformBuffer, nullptr,
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

			/*	*/
			ModelImporter modelLoader(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);
			this->skeleton = modelLoader.getSkeletons()[0];
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {
			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_skeleton_buffer_binding,
								  this->uniform_skeleton_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSkeletonBufferSize,
								  this->uniformSkeletonBufferSize);

				glUseProgram(this->skinned_graphic_program);

				this->scene.render();

				/*	*/
				if (this->skinnedSettingComponent->showBone) {
					glUseProgram(this->skinned_bone_program);
					glUseProgram(0);
				}

				/*	*/
				if (this->skinnedSettingComponent->showWeight) {
					glUseProgram(this->skinned_debug_weight_program);
					glDepthFunc(GL_LEQUAL);

					scene.render();
					glUseProgram(0);
				}

				/*	*/
				if (this->skinnedSettingComponent->showAxis) {
					glUseProgram(this->axis_orientation_program);

					/*	Draw all bone points.	*/

					glUseProgram(0);
				}

				glDisable(GL_CULL_FACE);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());
			this->scene.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_skeleton_buffer);
				glm::mat4 *uniformPointer = (glm::mat4 *)glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSkeletonBufferSize,

					this->uniformSkeletonBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				/*	*/
				int i = 0;
				for (auto it = skeleton.bones.begin(); it != skeleton.bones.end(); it++) {

					memcpy(&uniformPointer[i++], &(*it).second.inverseBoneMatrix[0][0], sizeof(uniformPointer[0]));
				}
			}
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SkinnedMeshGLSample : public GLSample<SkinnedMesh> {
	  public:
		SkinnedMeshGLSample() : GLSample<SkinnedMesh>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/Offensive.fbx"));
		}
	};
}; // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SkinnedMeshGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
