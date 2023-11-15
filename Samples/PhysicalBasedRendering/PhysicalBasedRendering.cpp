#include "Scene.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/glm.hpp>
#include <iostream>

namespace glsample {

	class PBRScene : public Scene {
	  public:
	};

	/**
	 * @brief
	 *
	 */
	class PhysicalBasedRendering : public GLSampleWindow {
	  public:
		PhysicalBasedRendering() : GLSampleWindow() {

			this->physicalBasedRenderingSettingComponent =
				std::make_shared<PhysicalBasedRenderingSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(physicalBasedRenderingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		unsigned int reflection_texture;

		/*	*/
		Scene scene;
		Skybox skybox;

		unsigned int physical_based_rendering_program;
		unsigned int simple_physical_based_rendering_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t skyboxUniformSize = 0;

		const NodeObject *rootNode;
		CameraController camera;

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			glm::vec4 diffuseColor;

		} uniform_stage_buffer;

		typedef struct MaterialUniformBlock_t {

		} MaterialUniformBlock;

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.vert.spv";
		const std::string fragmentPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.frag.spv";

		/*	Advanced.	*/
		const std::string vertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert.spv";
		const std::string fragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag.spv";
		const std::string ControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc.spv";
		const std::string EvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese.spv";

		class PhysicalBasedRenderingSettingComponent : public nekomimi::UIComponent {

		  public:
			PhysicalBasedRenderingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Physical Based Rendering Settings");
			}
			void draw() override {
				/*	*/
				ImGui::TextUnformatted("Skybox");
				// ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				/*	*/
				ImGui::TextUnformatted("Debug");

				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};

		std::shared_ptr<PhysicalBasedRenderingSettingComponent> physicalBasedRenderingSettingComponent;

		void Release() override { glDeleteProgram(this->physical_based_rendering_program); }

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			this->setTitle(fmt::format("Physical Based Rendering: {}", modelPath));
			{
				/*	*/
				const std::vector<uint32_t> pbr_vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_control_source =
					IOUtil::readFileData<uint32_t>(this->ControlShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_evolution_source =
					IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, this->getFileSystem());

				const std::vector<uint32_t> pbr_base_vertex_binary =
					IOUtil::readFileData<uint32_t>(vertexPBRShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_base_fragment_binary =
					IOUtil::readFileData<uint32_t>(fragmentPBRShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->physical_based_rendering_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &pbr_vertex_source, &pbr_fragment_source, nullptr,
													 &pbr_control_source, &pbr_evolution_source);

				this->simple_physical_based_rendering_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &pbr_base_vertex_binary, &pbr_base_fragment_binary);

				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup shader.	*/
			glUseProgram(this->physical_based_rendering_program);
			int uniform_buffer_index =
				glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Albedo"), 0);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Normal"), 1);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "AmbientOcclusion"), 2);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "HightMap"), 3);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "Irradiance"), 4);
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			uniform_buffer_index = glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup shader.	*/
			glUseProgram(this->simple_physical_based_rendering_program);
			uniform_buffer_index =
				glGetUniformBlockIndex(this->simple_physical_based_rendering_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->simple_physical_based_rendering_program, "Albedo"), 0);
			glUniform1iARB(glGetUniformLocation(this->simple_physical_based_rendering_program, "Normal"), 1);
			glUniform1iARB(glGetUniformLocation(this->simple_physical_based_rendering_program, "AmbientOcclusion"), 2);
			glUniform1iARB(glGetUniformLocation(this->simple_physical_based_rendering_program, "HightMap"), 3);
			glUniform1iARB(glGetUniformLocation(this->simple_physical_based_rendering_program, "Irradiance"), 4);
			glUniformBlockBinding(this->simple_physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			uniform_buffer_index = glGetUniformBlockIndex(this->simple_physical_based_rendering_program, "UniformBufferBlock");
			glUniformBlockBinding(this->simple_physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);
			/*	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->reflection_texture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(this->reflection_texture, this->skybox_program);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = PBRScene::loadFrom(modelLoader);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			glClear(GL_COLOR_BUFFER_BIT);

			{
				glUseProgram(this->physical_based_rendering_program);

				scene.render();
			}

			this->skybox.Render(this->camera);
		}

		void update() override {
			/*	*/
			camera.update(getTimer().deltaTime());

			this->uniform_stage_buffer.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.modelViewProjection = (this->uniform_stage_buffer.proj * camera.getViewMatrix());

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->skyboxUniformSize,
				this->skyboxUniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class PhysicalBasedRenderingGLSample : public GLSample<PhysicalBasedRendering> {
	  public:
		PhysicalBasedRenderingGLSample() : GLSample<PhysicalBasedRendering>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"T,skybox-texture", "Skybox Texture Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PhysicalBasedRenderingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
