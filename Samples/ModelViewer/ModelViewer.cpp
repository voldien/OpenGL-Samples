#include "ModelViewer.h"
#include "ImageImport.h"
#include "ModelImporter.h"
#include "Scene.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>

namespace glsample {

	/**
	 * @brief
	 */
	ModelViewer::ModelViewer() {
		this->setTitle("Model Viewer");

		this->modelviewerSettingComponent = std::make_shared<ModelViewerSettingComponent>(*this);
		this->addUIComponent(this->modelviewerSettingComponent);

		/*	Default camera position and orientation.	*/
		this->cameraController.setPosition(glm::vec3(20.5f));
		this->cameraController.lookAt(glm::vec3(0.f));
	}

	void ModelViewer::Release() {}

	void ModelViewer::Initialize() {
		/*	*/
		const std::string modelPath = this->getResult()["model"].as<std::string>();
		const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

		{
			/*	*/
			const std::vector<uint32_t> pbr_vertex_binary =
				IOUtil::readFileData<uint32_t>(this->PBRvertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> pbr_fragment_binary =
				IOUtil::readFileData<uint32_t>(this->PBRfragmentShaderPath, this->getFileSystem());
			const std::vector<uint32_t> pbr_control_binary =
				IOUtil::readFileData<uint32_t>(this->PBRControlShaderPath, this->getFileSystem());
			const std::vector<uint32_t> pbr_evolution_binary =
				IOUtil::readFileData<uint32_t>(this->PBREvoluationShaderPath, this->getFileSystem());

			/*	Setup compiler convert options.	*/
			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->physical_based_rendering_program = ShaderLoader::loadGraphicProgram(
				compilerOptions, &pbr_vertex_binary, &pbr_fragment_binary, nullptr, nullptr, nullptr);

			/*	Create skybox graphic pipeline program.	*/
			this->skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());
		}

		/*	Setup graphic pipeline.	*/
		glUseProgram(this->physical_based_rendering_program);
		int uniform_buffer_index = glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");

		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DiffuseTexture"),
					   (int)TextureType::Diffuse);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "NormalTexture"),
					   (int)TextureType::Normal);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "AOTexture"),
					   (int)TextureType::AmbientOcclusion);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "RoughnessTexture"),
					   (int)TextureType::Specular);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "DisplacementTexture"),
					   (int)TextureType::Displacement);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "EmissionTexture"),
					   (int)TextureType::Emission);
		glUniform1iARB(glGetUniformLocation(this->physical_based_rendering_program, "IrradianceTexture"),
					   (int)TextureType::Irradiance);
		glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index, 0);

		glUseProgram(0);

		/*	load Textures	*/
		TextureImporter textureImporter(this->getFileSystem());
		unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
		this->skybox.Init(skytexture, this->skybox_program);

		ProcessData util(this->getFileSystem());
		util.computeIrradiance(skytexture, this->irradiance_texture, 256, 128);

		/*	*/
		ModelImporter *modelLoader = new ModelImporter(this->getFileSystem());
		modelLoader->loadContent(modelPath, 0);
		this->scene = Scene::loadFrom(*modelLoader);

		/*	Align uniform buffer in respect to driver requirement.	*/
		GLint minMapBufferSize = 0;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
		this->uniformAlignBufferSize =
			fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

		/*	Create uniform buffer.	*/
		glGenBuffers(1, &this->uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void ModelViewer::draw() {

		int width = 0, height = 0;
		this->getSize(&width, &height);

		/*	*/
		glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
						  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
						  this->uniformAlignBufferSize);

		/*	Set render viewport size in pixels.	*/
		glViewport(0, 0, width, height);
		/*	Clear default framebuffer color attachment.	*/
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		{
			glActiveTexture(GL_TEXTURE0 + TextureType::Irradiance);
			glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

			glCullFace(GL_BACK);

			/*	Bind shader pipeline.	*/
			glUseProgram(this->physical_based_rendering_program);
			this->scene.render(&this->cameraController);
			glUseProgram(0);
		}

		this->skybox.Render(this->cameraController);
	}

	void ModelViewer::update() {
		/*	Update Camera.	*/
		this->cameraController.update(this->getTimer().deltaTime<float>());
		this->scene.update(this->getTimer().deltaTime<float>());

		/*	*/
		{
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(1.0f));

			this->uniformStageBuffer.view = this->cameraController.getViewMatrix();
			this->uniformStageBuffer.proj = this->cameraController.getProjectionMatrix();
			this->uniformStageBuffer.viewProjection = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
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
	}

} // namespace glsample
