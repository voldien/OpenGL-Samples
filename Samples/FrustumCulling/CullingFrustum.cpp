#include "Core/Math3D.h"
#include "Scene.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SceneFrustum : Scene {};
	/**
	 * FrustumCulling Rendering Path Sample.
	 **/
	class FrustumCulling : public GLSampleWindow {
	  public:
		FrustumCulling() : GLSampleWindow() {
			this->setTitle("FrustumCulling Rendering");

			/*	Setting Window.	*/
			this->frustumCullingSettingComponent =
				std::make_shared<FrustumCullingSettingComponent>(this->uniformBuffer);
			this->addUIComponent(this->frustumCullingSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

		} uniformBuffer;

		typedef struct material_t {

		} Material;

		typedef struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;
		} PointLight;

		std::vector<PointLight> pointLights;

		/*	*/
		GeometryObject boundingBox;
		GeometryObject frustum;
		Scene scene; /*	World Scene.	*/

		/*	*/
		unsigned int deferred_program;
		unsigned int skybox_program;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_pointlight_buffer_binding = 1;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t uniformLightBufferSize = sizeof(UniformBufferBlock);

		/*	*/
		CameraController camera;
		CameraController camera2;

		/*	FrustumCulling Rendering Path.	*/
		const std::string vertexShaderPath = "Shaders/deferred/deferred.vert.spv";
		const std::string fragmentShaderPath = "Shaders/deferred/deferred.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		class FrustumCullingSettingComponent : public nekomimi::UIComponent {
		  public:
			FrustumCullingSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
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
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<FrustumCullingSettingComponent> frustumCullingSettingComponent;

		void Release() override {
			glDeleteProgram(this->deferred_program);
			glDeleteProgram(this->skybox_program);

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

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

				/*	Load shader	*/
				this->deferred_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->deferred_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->deferred_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->deferred_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->deferred_program, "NormalTexture"), 1);
			glUniform1i(glGetUniformLocation(this->deferred_program, "WorldTexture"), 2);
			glUniformBlockBinding(this->deferred_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*	Load Light geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateSphere(1, vertices, indices);
				ProceduralGeometry::generatePlan(1, vertices, indices);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->frustum.vao);
				glBindVertexArray(this->frustum.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->frustum.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, this->frustum.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->frustum.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->frustum.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->frustum.nrIndicesElements = indices.size();

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
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	Draw from main camera */
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glDepthMask(GL_TRUE);
				glDisable(GL_CULL_FACE);

				this->scene.render();

				glUseProgram(0);
			}

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
							  this->uniformBufferSize);

			/*	Draw from second camera */
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(width * 0.7, height * 0.7, width * 0.3, height * 0.3);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glDepthMask(GL_TRUE);
				glDisable(GL_CULL_FACE);

				this->scene.render();

				glUseProgram(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			{
				this->uniformBuffer.proj = this->camera.getProjectionMatrix();
				this->uniformBuffer.model = glm::mat4(1.0f);
				this->uniformBuffer.model = glm::rotate(this->uniformBuffer.model, glm::radians(elapsedTime * 45.0f),
														glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(1.95f));
				this->uniformBuffer.view = this->camera.getViewMatrix();
				this->uniformBuffer.modelViewProjection =
					this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			}

			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class FrustumCullingGLSample : public GLSample<FrustumCulling> {
	  public:
		FrustumCullingGLSample() : GLSample<FrustumCulling>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Texture Path",
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