#include "Scene.h"
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
	 * Simple Fog Sample.
	 */
	class SimpleFog : public GLSampleWindow {
	  public:
		SimpleFog() : GLSampleWindow() {
			this->setTitle("Fog - Rasterize Fog");

			/*	Setting Window.	*/
			this->fogSettingComponent = std::make_shared<FogSettingComponent>(*this);
			this->addUIComponent(this->fogSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

			/*	Material.	*/
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1.0f);

			/*	Fog.	*/
			glm::vec4 fogColor = glm::vec4(1, 0, 0, 1);
			float cameraNear = 0.15f;
			float cameraFar = 1000.0f;
			float fogStart = 100;
			float fogEnd = 1000;
			float fogDensity = 0.1f;
			FogType fogType = FogType::Exp;
			float fogIntensity = 1.0f;
		} uniform_stage_buffer;

		Scene scene;

		unsigned int graphic_fog_program = 0;

		/*	Uniform Buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class FogSettingComponent : public GLUIComponent<SimpleFog> {
		  public:
			FogSettingComponent(SimpleFog &sample)
				: GLUIComponent(sample, "Fog Settings"), uniform(this->getRefSample().uniform_stage_buffer) {}

			void draw() override {
				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Fog Settings");
				ImGui::DragInt("Fog Type", (int *)&this->uniform.fogType);
				ImGui::ColorEdit4("Fog Color", &this->uniform.fogColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat("Fog Density", &this->uniform.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->uniform.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->uniform.fogStart);
				ImGui::DragFloat("Fog End", &this->uniform.fogEnd);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);

				this->getRefSample().scene.renderUI();
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<FogSettingComponent> fogSettingComponent;

		/*	Shader Paths.	*/
		const std::string vertexGraphicShaderPath = "Shaders/fog/fog.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/fog/fog.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_fog_program);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shaders	*/
				this->graphic_fog_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
			}

			/*	*/
			glUseProgram(this->graphic_fog_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_fog_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_fog_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->graphic_fog_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);
		}

		void onResize(int width, int height) override {
			/*	*/
			this->camera.setFar(2000.0f);
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.09f, 0.09f, 0.09f, 1.0f);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->fogSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	*/
			{

				// glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
				// 				  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				// 				  this->uniformAlignBufferSize);

				glUseProgram(this->graphic_fog_program);

				/*	*/
				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);

				this->scene.render(&this->camera);
			}
		}

		void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			this->scene.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniform_stage_buffer.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.model = glm::scale(this->uniform_stage_buffer.model, glm::vec3(0.5f));
			this->uniform_stage_buffer.view = this->camera.getViewMatrix();
			this->uniform_stage_buffer.modelViewProjection =
				this->uniform_stage_buffer.proj * this->uniform_stage_buffer.view * this->uniform_stage_buffer.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class FogGLSample : public GLSample<SimpleFog> {
	  public:
		FogGLSample() : GLSample<SimpleFog>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::FogGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}