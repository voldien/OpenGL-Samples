#include "ModelViewer.h"
#include "ImageImport.h"
#include "ModelImporter.h"
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

		this->modelviewerSettingComponent = std::make_shared<ModelViewerSettingComponent>(this->uniformStageBuffer);
		this->addUIComponent(this->modelviewerSettingComponent);

		/*	Default camera position and orientation.	*/
		this->camera.setPosition(glm::vec3(-2.5f));
		this->camera.lookAt(glm::vec3(0.f));
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

			const std::vector<uint32_t> vertex_skybox_binary =
				IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_skybox_binary =
				IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			/*	Setup compiler convert options.	*/
			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->physical_based_rendering_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &pbr_vertex_binary, &pbr_fragment_binary, nullptr,
												 &pbr_control_binary, &pbr_evolution_binary);

			/*	Create skybox graphic pipeline program.	*/
			this->skybox_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
		}

		/*	Setup graphic pipeline.	*/
		glUseProgram(this->skybox_program);
		int uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
		glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
		glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
		glUseProgram(0);

		/*	Setup graphic pipeline.	*/
		glUseProgram(this->physical_based_rendering_program);
		uniform_buffer_index = glGetUniformBlockIndex(this->physical_based_rendering_program, "UniformBufferBlock");

		glUniform1i(glGetUniformLocation(this->physical_based_rendering_program, "albedoMap"), 0);
		glUniform1i(glGetUniformLocation(this->physical_based_rendering_program, "normalMap"), 1);
		glUniformBlockBinding(this->physical_based_rendering_program, uniform_buffer_index, 0);

		glUseProgram(0);

		/*	load Textures	*/
		TextureImporter textureImporter(this->getFileSystem());
		unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
		this->skybox.Init(skytexture, this->skybox_program);

		/*	*/
		ModelImporter *modelLoader = new ModelImporter(this->getFileSystem());
		modelLoader->loadContent(modelPath, 0);
		this->scene = Scene::loadFrom(*modelLoader);

		/*	Align uniform buffer in respect to driver requirement.	*/
		GLint minMapBufferSize;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
		this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

		/*	Create uniform buffer.	*/
		glGenBuffers(1, &this->uniform_buffer);
		glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
		glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void ModelViewer::draw() {

		int width, height;
		this->getSize(&width, &height);

		/*	*/
		glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
						  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		/*	Set render viewport size in pixels.	*/
		glViewport(0, 0, width, height);
		/*	Clear default framebuffer color attachment.	*/
		glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			/*	Disable depth and culling of faces.	*/
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);

			/*	Bind shader pipeline.	*/
			glUseProgram(this->physical_based_rendering_program);

			this->scene.render();

			glUseProgram(0);
		}

		this->skybox.Render(this->camera);
	}

	void ModelViewer::update() {
		/*	Update Camera.	*/
		const float elapsedTime = this->getTimer().getElapsed<float>();
		this->camera.update(this->getTimer().deltaTime<float>());

		/*	*/
		{
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::rotate(
				this->uniformStageBuffer.model, glm::radians(elapsedTime * 12.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(2.95f));

			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
			this->uniformStageBuffer.viewProjection = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			this->uniformStageBuffer.camera.gEyeWorldPos = glm::vec4(this->camera.getPosition(), 0);
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

} // namespace glsample
