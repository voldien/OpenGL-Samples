#include "ModelViewer.h"
#include "PBRScene.h"
#include "Scene/Scene.h"
#include "Scene/SceneHelper.h"
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

	ModelViewer::ModelViewer() : GLSampleWindow() {

		/*	*/
		this->modelviewerSettingComponent = std::make_shared<ModelViewerSettingComponent>(*this);
		this->addUIComponent(modelviewerSettingComponent);

		/*	Default camera position and orientation.	*/
		this->camera.setPosition(glm::vec3(50.5f));
		this->camera.lookAt(glm::vec3(0.f));
	}

	void ModelViewer::Release() { glDeleteProgram(this->physical_based_rendering_program); }

	void ModelViewer::Initialize() {

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
		int uniform_buffer_index = glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DiffuseTexture"),
					   (int)TextureTypeBinding::Diffuse);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "NormalTexture"),
					   (int)TextureTypeBinding::Normal);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "AOTexture"),
					   (int)TextureTypeBinding::AmbientOcclusion);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DisplacementTexture"),
					   (int)TextureTypeBinding::Displacement);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "IrradianceTexture"),
					   (int)TextureTypeBinding::Irradiance);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "prefilterMap"),
					   (int)TextureTypeBinding::PreFilter);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "BRDFLUT"),
					   (int)TextureTypeBinding::BRDFLUT);
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
		MiscProcessingUtil util(this->getFileSystem());
		util.computeDiffuseIrradiance(skybox.getTexture(), this->irradiance_texture, 256, 128);
		util.computeReflectanceIrradiance(skybox.getTexture(), this->reflection_prefilter_texture, 2048, 1024);
		util.computeBRDFIntegrationMap(this->brdf_integration_map_texture, 512, 512);

		glFlush();

		/*	*/
		ModelImporter modelLoader(FileSystem::getFileSystem());
		modelLoader.loadContent(modelPath, 0);
		this->scene = SceneHelper::loadFrom<PBRScene>(modelLoader);
	}

	void ModelViewer::onResize(int width, int height) { this->camera.setAspect((float)width / (float)height); }

	void ModelViewer::draw() {

		/*	Shadow Pass.	*/
		{ this->scene.shadowPass(); }

		this->scene.update(this->getTimer().deltaTime<float>());
		{
			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			size_t width = 0, height = 0;
			this->getCurrentFrameBufferSize(&width, &height);

			/*	Set render viewport size in pixels.	*/
			glViewport(0, 0, width, height);
			glClear(GL_DEPTH_BUFFER_BIT);

			{
				glActiveTexture(GL_TEXTURE0 + TextureTypeBinding::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				glActiveTexture(GL_TEXTURE0 + TextureTypeBinding::BRDFLUT);
				glBindTexture(GL_TEXTURE_2D, this->brdf_integration_map_texture);

				glActiveTexture(GL_TEXTURE0 + TextureTypeBinding::PreFilter);
				glBindTexture(GL_TEXTURE_2D, this->reflection_prefilter_texture);

				glUseProgram(this->physical_based_rendering_program);
				this->scene.render(&this->camera);
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);
		}
	}

	void ModelViewer::update() {
		/*	*/
		this->camera.update(getTimer().deltaTime<float>());
	}

	class PhysicalBasedRenderingGLSample : public GLSample<ModelViewer> {
	  public:
		PhysicalBasedRenderingGLSample() : GLSample<ModelViewer>() {}
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
