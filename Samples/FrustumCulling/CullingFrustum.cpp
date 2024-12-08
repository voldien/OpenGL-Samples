#include "Common.h"
#include "GLSampleSession.h"
#include "GeometryUtil.h"
#include "Math/Math.h"
#include "Math3D/BoundingSphere.h"
#include "Math3D/Math3D.h"
#include "Scene.h"
#include "Skybox.h"
#include "Util/CameraController.h"
#include "Util/Frustum.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <queue>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SceneFrustum : public Scene {
	  public:
		SceneFrustum() = default;

		void render() override { Scene::render(); }

		void render(std::queue<const NodeObject *> &queue) {
			while (!queue.empty()) {
				Scene::renderNode(queue.front());
				queue.pop();
			}
		}

		void renderNode(const NodeObject *node) override { Scene::renderNode(node); }
	};

	/**
	 * @brief FrustumCulling
	 *
	 */
	class FrustumCulling : public GLSampleWindow {
	  public:
		FrustumCulling() : GLSampleWindow() {
			this->setTitle("FrustumCulling");

			/*	Setting Window.	*/
			this->frustumCullingSettingComponent =
				std::make_shared<FrustumCullingSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->frustumCullingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));

			/*	*/
			this->camera_observe_frustum.setPosition(glm::vec3(100, 1000.5f, 100));
			this->camera_observe_frustum.lookAt(glm::vec3(0.f));

			/*	*/
			this->camera_observe_frustum.enableNavigation(false);
			this->camera_observe_frustum.enableLook(false);
		}

		using UniformBufferBlock = struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 ViewProj{};
			glm::mat4 modelViewProjection{};

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
		};

		UniformBufferBlock uniformStageBuffer;

		/*	*/
		MeshObject boundingBox;
		MeshObject frustum;
		MeshObject plan;
		MeshObject frustumPlanes;
		SceneFrustum scene; /*	World Scene.	*/
		Skybox skybox;

		/*	*/
		unsigned int graphic_program{};
		unsigned int bounding_program{};
		unsigned int wireframe_program{};
		unsigned int hyperplane_program{};

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_instance_buffer_binding = 1;
		unsigned int uniform_buffer{};
		unsigned int uniform_instance_buffer{};
		const size_t nrUniformBuffers = 3;
		const size_t nrCameras = 2;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t uniformInstanceSize = 0;

		/*	*/
		CameraController camera;
		CameraController camera_observe_frustum;

		std::queue<const NodeObject *> mainCameraNodeQueue;
		std::queue<const NodeObject *> secondCameraNodeQueue;

		size_t instanceBatch = 0;

		/*	FrustumCulling Rendering Path.	*/
		const std::string vertexShaderPath = "Shaders/texture/texture.vert.spv";
		const std::string fragmentShaderPath = "Shaders/texture/texture.frag.spv";

		const std::string vertexBoundingShaderPath = "Shaders/bounding/boundingbox.vert.spv";
		const std::string fragmentBoundingShaderPath = "Shaders/bounding/boundingbox.frag.spv";

		const std::string vertexHyperplaneShaderPath = "Shaders/svm/hyperplane_simple.vert.spv";
		const std::string fragmentHyperplaneShaderPath = "Shaders/svm/hyperplane_simple.frag.spv";

		const std::string vertexBlendShaderPath = "Shaders/blending/blending.vert.spv";
		const std::string fragmentBlendShaderPath = "Shaders/blending/blending.frag.spv";

		class FrustumCullingSettingComponent : public nekomimi::UIComponent {
		  public:
			FrustumCullingSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("FrustumCulling Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("Show Frustum", &this->showFrustum);
				ImGui::Checkbox("Show Bounds In View", &this->showBoundsInMainView);
				ImGui::Checkbox("Show Bounds in Second Camera", &this->showBoundIn2Camera);
				ImGui::Checkbox("Show Second Camera View", &this->showSecondCameraView);
				ImGui::Checkbox("Frustum Culling", &this->useFrustumCulling);
				ImGui::Checkbox("Sphere Culling", &this->useSphereCulling);
			}

			bool useFrustumCulling = true;
			bool showFrustum = true;
			bool showBoundIn2Camera = true;
			bool showBoundsInMainView = true;
			bool showSecondCameraView = true;
			bool useSphereCulling = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<FrustumCullingSettingComponent> frustumCullingSettingComponent;

		void Release() override {

			this->scene.release();

			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->bounding_program);
			glDeleteProgram(this->wireframe_program);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);
			glDeleteBuffers(1, &this->uniform_instance_buffer);

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

				const std::vector<uint32_t> vertex_graphic_binary =
					IOUtil::readFileData<uint32_t>(vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_graphic_binary =
					IOUtil::readFileData<uint32_t>(fragmentShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_bound_binary =
					IOUtil::readFileData<uint32_t>(this->vertexBoundingShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_bound_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBoundingShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_blending_binary =
					IOUtil::readFileData<uint32_t>(this->vertexBlendShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blending_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBlendShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_binary);

				/*	Load shader	*/
				this->wireframe_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_bound_binary, &fragment_bound_binary);

				/*	Load shader	*/
				this->bounding_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_bound_binary, &fragment_bound_binary);
			}

			/*	Setup graphic program.	*/
			glUseProgram(this->graphic_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, uniform_buffer_binding);
			glUniform1i(glGetUniformLocation(this->graphic_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Setup graphic program.	*/
			glUseProgram(this->bounding_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->bounding_program, "UniformBufferBlock");
			int uniform_instance_buffer_index = glGetUniformBlockIndex(this->bounding_program, "UniformInstanceBlock");
			glUniformBlockBinding(this->bounding_program, uniform_buffer_index, uniform_buffer_binding);
			glUniformBlockBinding(this->bounding_program, uniform_instance_buffer_index,
								  uniform_instance_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffers * this->nrCameras,
						 nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Setup instance buffer.	*/
			{
				/*	*/
				GLint uniformMaxSize = 0;
				glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniformMaxSize);
				this->instanceBatch = std::min<size_t>(uniformMaxSize / sizeof(glm::mat4), 512);

				this->uniformInstanceSize =
					fragcore::Math::align<size_t>(this->instanceBatch * sizeof(glm::mat4), (size_t)minMapBufferSize);
				glGenBuffers(1, &this->uniform_instance_buffer);
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
				glBufferData(GL_UNIFORM_BUFFER, this->uniformInstanceSize * this->nrUniformBuffers, nullptr,
							 GL_DYNAMIC_DRAW);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom<SceneFrustum>(modelLoader);

			/*	load Skybox Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, Skybox::loadDefaultProgram(this->getFileSystem()));

			/*	Load Light geometry.	*/
			{
				Common::loadCube(boundingBox, 1);

				std::vector<ProceduralGeometry::Vertex> frustumVertices;
				/*	Update frustum geometry.	*/
				const Matrix4x4 proj = GLM2E(camera.getProjectionMatrix());
				ProceduralGeometry::createFrustum(frustumVertices, proj);
			}
		}

		void onResize(int width, int height) override {

			/*	Update camera aspect	*/
			this->camera.setAspect((float)width / (float)height);
			this->camera_observe_frustum.setAspect((float)width / (float)height);

			this->camera.setFar(2000.0f);
			this->camera_observe_frustum.setFar(2000.0f);

			/*	Update frustum geometry.	*/
			const Matrix4x4 proj = GLM2E(camera.getProjectionMatrix());
			std::vector<ProceduralGeometry::Vertex> frustumVertices;
			ProceduralGeometry::createFrustum(frustumVertices, proj);
		}

		void draw() override {

			/*	*/
			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformAlignBufferSize *
								  this->nrCameras,
							  this->uniformAlignBufferSize);

			/*	*/
			this->secondCameraNodeQueue = this->mainCameraNodeQueue;

			/*	Draw from main camera */
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);
				glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glUseProgram(this->graphic_program);
				this->scene.render(this->mainCameraNodeQueue);
				glUseProgram(0);
				this->skybox.Render(this->camera);

				if (this->frustumCullingSettingComponent->showBoundsInMainView) {
					auto copyNode = this->secondCameraNodeQueue;
					glUseProgram(this->bounding_program);
					this->renderBoundingBox(this->camera, copyNode);
					glUseProgram(0);
				}
			}

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformAlignBufferSize *
									  this->nrCameras +
								  this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			if (this->frustumCullingSettingComponent->showSecondCameraView) {

				/*	Draw from second camera */
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					/*	*/
					const int subX = width * 0.7;
					const int subY = height * 0.7;
					const int subWidth = width * 0.3;
					const int subHeight = height * 0.3;

					glViewport(subX, subY, subWidth, subHeight);
					glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
					glScissor(subX, subY, subWidth, subHeight);
					glEnable(GL_SCISSOR_TEST);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					glUseProgram(this->graphic_program);

					auto copy_observer_camera_node = this->secondCameraNodeQueue;
					this->scene.render(copy_observer_camera_node);
					glUseProgram(0);
					this->skybox.Render(this->camera_observe_frustum);

					if (this->frustumCullingSettingComponent->showFrustum) {
						glUseProgram(this->bounding_program);
						this->renderFrustum(this->camera_observe_frustum);
						glUseProgram(0);
					}

					if (this->frustumCullingSettingComponent->showBoundIn2Camera) {
						glUseProgram(this->bounding_program);
						this->renderBoundingBox(this->camera_observe_frustum, this->secondCameraNodeQueue);
						glUseProgram(0);
					}

					glDisable(GL_SCISSOR_TEST);
				}
			}
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			this->camera_observe_frustum.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			}

			{
				UniformBufferBlock copy = this->uniformStageBuffer;

				copy.proj = this->camera_observe_frustum.getProjectionMatrix();
				copy.view = this->camera_observe_frustum.getViewMatrix();
				copy.modelViewProjection = copy.proj * copy.view * copy.model;

				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffers) * this->uniformAlignBufferSize *
						this->nrCameras,
					this->uniformAlignBufferSize * this->nrCameras, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

				/*	*/
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				memcpy(&uniformPointer[this->uniformAlignBufferSize], &copy, sizeof(copy));

				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}

			/*	Compute frustum culling.	*/
			mainCameraNodeQueue = std::queue<const NodeObject *>();
			secondCameraNodeQueue = std::queue<const NodeObject *>();
			for (size_t x = 0; x < this->scene.getNodes().size(); x++) {

				const NodeObject *node = this->scene.getNodes()[x];

				for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

					/*	Compute world space AABB.	*/
					const AABB aabb = GeometryUtility::computeBoundingBox(
						fragcore::AABB::createMinMax(
							Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
							Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2])),
						GLM2E<float, 4, 4>(node->modelGlobalTransform));

					if (this->frustumCullingSettingComponent->useSphereCulling) {
						BoundingSphere sphere = BoundingSphere(aabb.getCenter(), aabb.getSize().norm());

						if (this->camera.intersectionSphere(sphere) == Frustum::In ||
							!this->frustumCullingSettingComponent->useFrustumCulling) {
							/*	*/
							mainCameraNodeQueue.push(node);
						}

						if (this->camera_observe_frustum.intersectionSphere(sphere) == Frustum::In ||
							!this->frustumCullingSettingComponent->useFrustumCulling) {
							/*	*/
							secondCameraNodeQueue.push(node);
						}
					} else {

						if (this->camera.intersectionAABB(aabb) == Frustum::In ||
							!this->frustumCullingSettingComponent->useFrustumCulling) {
							/*	*/
							mainCameraNodeQueue.push(node);
						}

						if (this->camera_observe_frustum.intersectionAABB(aabb) == Frustum::In ||
							!this->frustumCullingSettingComponent->useFrustumCulling) {
							/*	*/
							secondCameraNodeQueue.push(node);
						}
					}
				}
			}

			this->getLogger().info("Culling Render {0}", mainCameraNodeQueue.size());
		}

		void renderFrustum(const CameraController &camera) {
			/*	*/
			glBindVertexArray(this->boundingBox.vao);

			glDrawElementsInstanced(GL_TRIANGLES, this->frustum.nrIndicesElements, GL_UNSIGNED_INT,
									(const void *)this->frustum.indices_offset, 1);
			glBindVertexArray(0);
		}

		void renderBoundingBox(const CameraController &camera, std::queue<const NodeObject *> &queue) {

			// TODO: fix if more than buffer size.
			std::vector<glm::mat4> instance_model_matrices;

			while (!queue.empty()) {

				const NodeObject *node = queue.front();
				queue.pop();

				for (size_t i = 0; i < node->geometryObjectIndex.size(); i++) {

					const MeshObject &refMesh = this->scene.getMeshes()[node->geometryObjectIndex[i]];

					/*	*/
					const AABB aabb = GeometryUtility::computeBoundingBox(
						fragcore::AABB::createMinMax(
							Vector3(node->bound.aabb.min[0], node->bound.aabb.min[1], node->bound.aabb.min[2]),
							Vector3(node->bound.aabb.max[0], node->bound.aabb.max[1], node->bound.aabb.max[2])),
						GLM2E<float, 4, 4>(node->modelGlobalTransform));

					glm::mat4 localBoundMatrix = glm::mat4(1);
					localBoundMatrix = glm::translate(localBoundMatrix, E2GLM<float, 3>(aabb.getCenter()));
					localBoundMatrix = glm::scale(localBoundMatrix, E2GLM<float, 3>(aabb.getSize()));

					const glm::mat4 model = node->modelGlobalTransform * localBoundMatrix;

					instance_model_matrices.push_back(model);
				}
			}

			/*	Transfer new */
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_instance_buffer);
				uint8_t *uniform_instance_buffer_pointer = static_cast<uint8_t *>(glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount()) % this->nrUniformBuffers) * this->uniformInstanceSize,
					this->uniformInstanceSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));

				/*	*/
				memcpy(uniform_instance_buffer_pointer, instance_model_matrices.data(),
					   instance_model_matrices.size() * sizeof(instance_model_matrices[0]));

				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}

			glBindVertexArray(this->boundingBox.vao);

			/*	*/
			glDisable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			/*	Blending.	*/
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  ((this->getFrameCount() % nrUniformBuffers)) * this->uniformAlignBufferSize *
								  this->nrCameras,
							  this->uniformAlignBufferSize);

			for (size_t x = 0; x < instance_model_matrices.size(); x += this->instanceBatch) {

				/*	*/
				const NodeObject *node = this->scene.getNodes()[x];
				const size_t nrDrawInstances = std::min(instance_model_matrices.size() - x, this->instanceBatch);

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_instance_buffer_binding,
								  this->uniform_instance_buffer,
								  (this->getFrameCount() % this->nrUniformBuffers) * this->uniformInstanceSize,
								  this->uniformInstanceSize);

				glDrawElementsInstanced(GL_TRIANGLES, this->boundingBox.nrIndicesElements, GL_UNSIGNED_INT, nullptr,
										nrDrawInstances);
			}

			glDepthMask(GL_TRUE);
			glBindVertexArray(0);
		}
	};

	class FrustumCullingGLSample : public GLSample<FrustumCulling> {
	  public:
		FrustumCullingGLSample() : GLSample<FrustumCulling>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
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