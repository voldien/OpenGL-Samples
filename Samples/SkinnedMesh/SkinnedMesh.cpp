#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

		} uniformStageBuffer;

		CameraController camera;

		unsigned int skinned_program;
		unsigned int skinned_debug_program;
		unsigned int skybox_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_pointlight_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_pointlight_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t uniformLightBufferSize = 0;

		/*	*/
		const std::string vertexSkinnedShaderPath = "Shaders/skinnedmesh/skinnedmesh.vert.spv";
		const std::string fragmentSkinnedShaderPath = "Shaders/skinnedmesh/skinnedmesh.frag.spv";

		const std::string vertexSkinnedDebugShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.vert.spv";
		const std::string fragmentSkinnedDebugShaderPath = "Shaders/skinnedmesh/skinnedmesh_debug.frag.spv";

		class SkinnedMeshSettingComponent : public nekomimi::UIComponent {

		  public:
			SkinnedMeshSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Skinned Mesh");
			}
			void draw() override {}

		  private:
			struct uniform_buffer_block &uniform;
			bool lightvisible[4] = {true, true, true, true};
		};
		std::shared_ptr<SkinnedMeshSettingComponent> skinnedSettingComponent;

		void Release() override {
			/*	*/
			glDeleteProgram(this->skinned_program);
			glDeleteProgram(this->skinned_debug_program);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {
			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> skinned_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkinnedShaderPath, this->getFileSystem());
				const std::vector<uint32_t> skinned_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkinnedShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> skinned_debug_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkinnedDebugShaderPath, this->getFileSystem());
				const std::vector<uint32_t> skinned_debug_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkinnedDebugShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->skinned_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &skinned_vertex_binary, &skinned_fragment_binary);

				/*	Load shader	*/
				this->skinned_program = ShaderLoader::loadGraphicProgram(compilerOptions, &skinned_debug_vertex_binary,
																		 &skinned_debug_fragment_binary);
			}
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
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);
				glUseProgram(this->skinned_program);

				glDisable(GL_CULL_FACE);
			}
		}

		void update() override {
			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{

				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(1.95f));
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			/*	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class SkinnedMeshGLSample : public GLSample<SkinnedMesh> {
	  public:
		SkinnedMeshGLSample() : GLSample<SkinnedMesh>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
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
