#include "Core/Math3D.h"
#include "Scene.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SceneFrustum : public Scene {
	  public:
		SceneFrustum() = default;

		void render() override { Scene::render(); }

		void renderBoundingBox(const CameraController &camera) {

			std::vector<glm::mat4> instance_model_matrices;
			for (size_t x = 0; x < this->nodes.size(); x++) {
				// Compute matrices.

				glm::mat4 model = glm::translate(glm::mat4(1.0), this->nodes[x]->position);
				model = (model * glm::toMat4(this->nodes[x]->rotation));
				model = glm::scale(model, this->nodes[x]->scale);

				instance_model_matrices[x] = model;
			}

			for (size_t x = 0; x < this->nodes.size(); x++) {

				/*	*/
				const NodeObject *node = this->nodes[x];
				this->renderBoundingBox(node);
			}
		}

	  private:
		void renderBoundingBox(const NodeObject *node) {
			/*	*/

			/*	*/
			// glBindVertexArray(this->refGeometry[0].vao);
			//
			///*	*/
			// glDrawElementsBaseVertex(
			//	GL_TRIANGLES, this->refGeometry[node->geometryObjectIndex[i]].nrIndicesElements, GL_UNSIGNED_INT,
			//	(void *)(sizeof(unsigned int) * this->refGeometry[node->geometryObjectIndex[i]].indices_offset),
			//	this->refGeometry[node->geometryObjectIndex[i]].vertex_offset);
			//
			// glBindVertexArray(0);
		}
	}; // namespace glsample

	/**
	 * @brief FrustumCulling
	 *
	 */
	class FrustumCulling : public GLSampleWindow {
	  public:
		FrustumCulling() : GLSampleWindow() {
			this->setTitle("FrustumCulling Rendering");

			/*	Setting Window.	*/
			this->frustumCullingSettingComponent =
				std::make_shared<FrustumCullingSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->frustumCullingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));

			this->camera2.setPosition(glm::vec3(200.5f));
			this->camera2.lookAt(glm::vec3(0.f));

			this->camera2.enableNavigation(false);
			this->camera2.enableLook(false);
		}

		typedef struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} UniformBufferBlock;

		UniformBufferBlock uniformStageBuffer;

		/*	*/
		GeometryObject boundingBox;
		GeometryObject frustum;
		SceneFrustum scene; /*	World Scene.	*/

		/*	*/
		unsigned int texture_program;
		unsigned int blending_program;
		unsigned int skybox_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffers = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t uniformLightBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;
		unsigned int uniform_instance_buffer;

		/*	*/
		CameraController camera;
		CameraController camera2;

		size_t instanceBatch = 0;

		/*	FrustumCulling Rendering Path.	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		const std::string vertexBlendShaderPath = "Shaders/blending/blending.vert.spv";
		const std::string fragmentBlendShaderPath = "Shaders/blending/blending.frag.spv";

		class FrustumCullingSettingComponent : public nekomimi::UIComponent {
		  public:
			FrustumCullingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("FrustumCulling Settings");
			}

			void draw() override {
				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Show Frustum", &this->showFrustum);
				ImGui::Checkbox("Show Bounds", &this->showBounds);
				ImGui::Checkbox("Show Frustum View", &this->showFrustumView);
			}

			bool showWireFrame = false;
			bool showFrustum = true;
			bool showBounds = true;
			bool showFrustumView = true;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<FrustumCullingSettingComponent> frustumCullingSettingComponent;

		void Release() override {

			this->scene.release();

			glDeleteProgram(this->texture_program);
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->blending_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->frustum.vao);
			glDeleteBuffers(1, &this->frustum.vbo);
			glDeleteBuffers(1, &this->frustum.ibo);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();

			{

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_blending_binary =
					IOUtil::readFileData<uint32_t>(this->vertexBlendShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blending_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBlendShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

				/*	Load shader	*/
				this->texture_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

				/*	Load shader	*/
				this->blending_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_blending_binary,
																		  &fragment_blending_binary);
			}

			/*	Setup graphic program.	*/
			glUseProgram(this->texture_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->texture_program, "UniformBufferBlock");
			glUniformBlockBinding(this->texture_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->texture_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Setup graphic program.	*/
			glUseProgram(this->blending_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->blending_program, "UniformBufferBlock");
			glUniformBlockBinding(this->blending_program, uniform_buffer_index, 0);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer); // TODO:fix constant
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffers * 2, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			GLint uniformMaxSize;
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniformMaxSize);
			this->instanceBatch = uniformMaxSize / sizeof(glm::mat4);

			/*	*/
			this->uniformInstanceSize =
				fragcore::Math::align(this->instanceBatch * sizeof(glm::mat4), (size_t)minMapBufferSize);
			glGenBuffers(1, &this->uniform_instance_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom<SceneFrustum>(modelLoader);

			/*	Load Light geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> planVertices;
				std::vector<unsigned int> planIndices;
				ProceduralGeometry::generateWireCube(1, planVertices, planIndices);

				std::vector<ProceduralGeometry::Vertex> frustumVertices;
				Matrix4x4 proj = Matrix4x4();
				camera.getProjectionMatrix();
				ProceduralGeometry::createFrustum(frustumVertices, proj);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->frustum.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, frustum.vbo);
				glBufferData(GL_ARRAY_BUFFER,
							 (planVertices.size() + frustumVertices.size()) * sizeof(ProceduralGeometry::Vertex),
							 nullptr, GL_STATIC_DRAW);
				glBufferSubData(GL_ARRAY_BUFFER, 0, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
								planVertices.data());
				glBufferSubData(GL_ARRAY_BUFFER, planVertices.size() * sizeof(ProceduralGeometry::Vertex),
								frustumVertices.size() * sizeof(ProceduralGeometry::Vertex), frustumVertices.data());

				/*	*/
				glGenBuffers(1, &this->frustum.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frustum.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, (planIndices.size()) * sizeof(planIndices[0]), nullptr,
							 GL_STATIC_DRAW);

				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, planIndices.size() * sizeof(planIndices[0]),
								planIndices.data());

				// this->frustum.nrIndicesElements = planIndices.size();
				// this->boundingBox.nrIndicesElements = sphereIndices.size();
				// this->boundingBox.vao = this->plan.vao;
				// this->boundingBox.indices_offset = planIndices.size();
				// this->boundingBox.vertex_offset = planVertices.size();

				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				/*	UV.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				/*	Normal.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}
		}

		void onResize(int width, int height) override {

			/*	Update camera	*/
			this->camera.setAspect((float)width / (float)height);
			this->camera2.setAspect((float)width / (float)height);

			/*	Update frustum geometry.	*/
			Matrix4x4 proj = Matrix4x4();
			camera.getProjectionMatrix();
			std::vector<ProceduralGeometry::Vertex> frustumVertices;
			ProceduralGeometry::createFrustum(frustumVertices, proj);
		}

		void draw() override {

			/*	*/
			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformBufferSize * 2,
							  this->uniformBufferSize);

			/*	Draw from main camera */
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->texture_program);

				this->scene.render();

				glUseProgram(0);

				if (this->frustumCullingSettingComponent->showBounds) {
					glUseProgram(this->blending_program);
					scene.renderBoundingBox(this->camera2);
					glUseProgram(0);
				}
			}

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformBufferSize * 2 +
								  this->uniformBufferSize,
							  this->uniformBufferSize);

			if (this->frustumCullingSettingComponent->showFrustumView) {

				/*	Draw from second camera */
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					/*	*/
					const int subX = width * 0.7;
					const int subY = height * 0.7;
					const int subWidth = width * 0.3;
					const int subHeight = height * 0.3;
					glViewport(subX, subY, subWidth, subHeight);
					glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
					glScissor(subX, subY, subWidth, subHeight);
					glEnable(GL_SCISSOR_TEST);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glUseProgram(this->texture_program);

					this->scene.render();

					glUseProgram(0);

					if (this->frustumCullingSettingComponent->showFrustum) {
						glUseProgram(this->blending_program);
						glUseProgram(0);
					}

					if (this->frustumCullingSettingComponent->showBounds) {
						glUseProgram(this->blending_program);
						scene.renderBoundingBox(this->camera2);
						glUseProgram(0);
					}

					glDisable(GL_SCISSOR_TEST);
				}
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();

			this->camera.update(this->getTimer().deltaTime<float>());
			this->camera2.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(1.95f));
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			{
				UniformBufferBlock copy = this->uniformStageBuffer;

				copy.proj = this->camera2.getProjectionMatrix();
				copy.view = this->camera2.getViewMatrix();
				copy.modelViewProjection = copy.proj * copy.view * copy.model;

				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformBufferSize * 2,
					this->uniformBufferSize * 2, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

				/*	*/
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				memcpy(&uniformPointer[this->uniformBufferSize], &copy, sizeof(copy));

				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class FrustumCullingGLSample : public GLSample<FrustumCulling> {
	  public:
		FrustumCullingGLSample() : GLSample<FrustumCulling>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::FrustumCullingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}