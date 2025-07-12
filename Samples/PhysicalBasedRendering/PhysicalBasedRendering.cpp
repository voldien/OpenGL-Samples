#include "GLUIComponent.h"
#include "SampleHelper.h"
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

			/*	*/
			this->physicalBasedRenderingSettingComponent =
				std::make_shared<PhysicalBasedRenderingSettingComponent>(*this);
			this->addUIComponent(physicalBasedRenderingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(50.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		unsigned int reflection_texture{};

		/*	*/
		PBRScene scene;
		Skybox skybox;

		unsigned int irradiance_texture{};

		unsigned int physical_based_rendering_program{};
		unsigned int simple_physical_based_rendering_program{};
		unsigned int skybox_program{};

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t skyboxUniformSize = 0;

		const NodeObject *rootNode{};
		CameraController camera;

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.vert.spv";
		const std::string fragmentPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.frag.spv";

		/*	Advanced.	*/
		const std::string vertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert.spv";
		const std::string fragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag.spv";
		const std::string ControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc.spv";
		const std::string EvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese.spv";

		class PhysicalBasedRenderingSettingComponent : public GLUIComponent<PhysicalBasedRendering> {

		  public:
			PhysicalBasedRenderingSettingComponent(PhysicalBasedRendering &uniform)
				: GLUIComponent<PhysicalBasedRendering>(uniform, "Physical Based Rendering Settings") {}
			void draw() override {
				ImGui::TextUnformatted("Debug");
				this->getRefSample().scene.renderUI();
			}

		  private:
		};

		std::shared_ptr<PhysicalBasedRenderingSettingComponent> physicalBasedRenderingSettingComponent;

		void Release() override { glDeleteProgram(this->physical_based_rendering_program); }

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			this->setTitle(fmt::format("Physical Based Rendering: {}", modelPath));

			{
				/*	*/
				const std::vector<uint32_t> pbr_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_control_binary =
					IOUtil::readFileData<uint32_t>(this->ControlShaderPath, this->getFileSystem());
				const std::vector<uint32_t> pbr_evolution_binary =
					IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, this->getFileSystem());

				// const std::vector<uint32_t> pbr_base_vertex_binary =
				//	IOUtil::readFileData<uint32_t>(vertexPBRShaderPath, this->getFileSystem());
				// const std::vector<uint32_t> pbr_base_fragment_binary =
				//	IOUtil::readFileData<uint32_t>(fragmentPBRShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				// this->physical_based_rendering_program =
				// 	ShaderLoader::loadGraphicProgram(compilerOptions, &pbr_vertex_binary, &pbr_fragment_binary, nullptr,
				// 									 &pbr_control_binary, &pbr_evolution_binary);

				this->physical_based_rendering_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &pbr_vertex_binary, &pbr_fragment_binary);

				this->skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());
			}

			/*	Setup shader.	*/
			glUseProgram(this->physical_based_rendering_program);
			int uniform_buffer_index =
				glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DiffuseTexture"),
						   (int)TextureType::Diffuse);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "NormalTexture"),
						   (int)TextureType::Normal);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "AOTexture"),
						   (int)TextureType::AmbientOcclusion);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DisplacementTexture"),
						   (int)TextureType::Displacement);
			glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "IrradianceTexture"),
						   (int)TextureType::Irradiance);
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			uniform_buffer_index = glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
			glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->reflection_texture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(this->reflection_texture, this->skybox_program);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = PBRScene::loadFrom<PBRScene>(modelLoader);

			MiscProcessingUtil util(this->getFileSystem());
			util.computeDiffuseIrradiance(skybox.getTexture(), this->irradiance_texture, 256, 128);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			size_t width = 0, height = 0;
			this->getCurrentFrameBufferSize(&width, &height);

			/*	Set render viewport size in pixels.	*/
			glViewport(0, 0, width, height);

			glClear(GL_DEPTH_BUFFER_BIT);

			{
				glActiveTexture(GL_TEXTURE0 + TextureType::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				glUseProgram(this->physical_based_rendering_program);
				this->scene.render(&this->camera);
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {
			/*	*/
			this->camera.update(getTimer().deltaTime<float>());
			this->scene.update(this->getTimer().deltaTime<float>());
		}
	};

	class PhysicalBasedRenderingGLSample : public GLSample<PhysicalBasedRendering> {
	  public:
		PhysicalBasedRenderingGLSample() : GLSample<PhysicalBasedRendering>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"T,skybox-texture", "Skybox Texture Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
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
