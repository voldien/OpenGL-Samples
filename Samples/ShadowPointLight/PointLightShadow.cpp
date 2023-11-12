#include "Scene.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class PointLightShadow : public GLSampleWindow {
	  public:
		PointLightShadow() : GLSampleWindow() {
			this->setTitle("PointLightShadow");
			this->shadowSettingComponent = std::make_shared<PointLightShadowSettingComponent>(this->uniform);
			this->addUIComponent(this->shadowSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		typedef struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;

			float bias = 0.01f;
			float shadowStrength = 1.0f;
			float padding0;
			float padding1;
		} PointLight;

		static const size_t nrPointLights = 4;
		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProjection[6];
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / std::sqrt(2.0f), -1.0f / std::sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 lightPosition;

			PointLight pointLights[nrPointLights];
			/*	*/
			glm::vec4 pcfFilters[20];
		} uniform;

		/*	Point light shadow maps.	*/
		std::vector<unsigned int> pointShadowFrameBuffers;
		std::vector<unsigned int> pointShadowTextures;

		/*	*/
		unsigned int shadowWidth = 1024;
		unsigned int shadowHeight = 1024;

		Scene scene;

		/*	*/
		unsigned int graphic_program;
		unsigned int graphic_pfc_program;
		unsigned int shadow_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class PointLightShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			PointLightShadowSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Point Light Shadow Settings");
			}
			void draw() override {

				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("PCF Shadow", &this->use_pcf);
				ImGui::TextUnformatted("Depth Texture");

				for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightvisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Light Color", &this->uniform.pointLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Light Position", &this->uniform.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
						ImGui::DragFloat("Light Range", &this->uniform.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
						ImGui::DragFloat("Shadow Strength", &this->uniform.pointLights[i].shadowStrength, 1, 0.0f,
										 1.0f);
						ImGui::DragFloat("Shadow Bias", &this->uniform.pointLights[i].bias, 1, 0.0f, 1.0f);
					}
					ImGui::PopID();
				}
				ImGui::Checkbox("Animate Lights", &this->animate);
			}

			bool showWireFrame = false;
			bool animate = false;
			bool use_pcf = false;

			bool lightvisible[4] = {true, true, true, true};

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<PointLightShadowSettingComponent> shadowSettingComponent;

		/*	Graphic shader paths.	*/
		const std::string vertexGraphicShaderPath = "Shaders/shadowpointlight/pointlightlight.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/shadowpointlight/pointlightlight.frag.spv";
		const std::string fragmentGraphicPCFShaderPath = "Shaders/shadowpointlight/pointlightlight_pcf.frag.spv";

		/*	Shadow shader paths.	*/
		const std::string vertexShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.vert.spv";
		const std::string geomtryShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.geom.spv";
		const std::string fragmentShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->shadow_program);
			glDeleteProgram(this->graphic_pfc_program);

			glDeleteFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());
			glDeleteTextures(this->pointShadowTextures.size(), this->pointShadowTextures.data());

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_pcf_source =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicPCFShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> vertex_shadow_source =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_shadow_source =
					IOUtil::readFileData<uint32_t>(this->geomtryShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_shadow_source =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shaders	*/
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);
				this->graphic_pfc_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_pcf_source);

				this->shadow_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_shadow_source, &fragment_shadow_source, &geometry_shadow_source);
			}

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());

			/*	Setup shadow graphic pipeline.	*/
			glUseProgram(this->shadow_program);
			int uniform_buffer_shadow_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shadow_program, uniform_buffer_shadow_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			const int shadows[4] = {2, 3, 4, 5};
			glUniform1iv(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 4, shadows);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_pfc_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->graphic_pfc_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_pfc_program, "DiffuseTexture"), 0);
			glUniform1iv(glGetUniformLocation(this->graphic_pfc_program, "ShadowTexture"), 4, shadows);
			glUniformBlockBinding(this->graphic_pfc_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	 Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer * this->nrPointLights,
						 nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				this->pointShadowFrameBuffers.resize(this->nrPointLights);
				glGenFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());

				this->pointShadowTextures.resize(this->nrPointLights);
				glGenTextures(this->pointShadowTextures.size(), this->pointShadowTextures.data());

				for (size_t x = 0; x < pointShadowFrameBuffers.size(); x++) {
					glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[x]);

					/*	*/
					glBindTexture(GL_TEXTURE_CUBE_MAP, this->pointShadowTextures[x]);

					/*	Setup each face.	*/
					for (size_t i = 0; i < 6; i++) {

						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, this->shadowWidth,
									 this->shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
					}

					/*	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					/*	Border clamped to max value, it makes the outside area.	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

					/*	*/
					float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
					glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);

					/*	*/
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
					glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 0.0f);
					glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);

					glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

					/*	*/
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->pointShadowTextures[x], 0);

					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);

					/*	*/
					int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					if (frstat != GL_FRAMEBUFFER_COMPLETE) {
						/*  Delete  */
						throw RuntimeException("Failed to create framebuffer, {}",
											   (const char *)glewGetErrorString(frstat));
					}
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrPointLights; i++) {
				this->uniform.pointLights[i].range = 25.0f;
				this->uniform.pointLights[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(2.0f);
				this->uniform.pointLights[i].color = colors[i];
				this->uniform.pointLights[i].constant_attenuation = 1.7f;
				this->uniform.pointLights[i].linear_attenuation = 1.5f;
				this->uniform.pointLights[i].qudratic_attenuation = 0.19f;
				this->uniform.pointLights[i].intensity = 1.0f;
				this->uniform.pointLights[i].shadowStrength = 1.0f;
				this->uniform.pointLights[i].bias = 0.01f;
			}

			/*	Copy PCF Filters	*/
			const glm::vec4 samples[20] = {
				glm::vec4(1, 1, 1, 0),	glm::vec4(1, -1, 1, 0),	 glm::vec4(-1, -1, 1, 0),  glm::vec4(-1, 1, 1, 0),
				glm::vec4(1, 1, -1, 0), glm::vec4(1, -1, -1, 0), glm::vec4(-1, -1, -1, 0), glm::vec4(-1, 1, -1, 0),
				glm::vec4(1, 1, 0, 0),	glm::vec4(1, -1, 0, 0),	 glm::vec4(-1, -1, 0, 0),  glm::vec4(-1, 1, 0, 0),
				glm::vec4(1, 0, 1, 0),	glm::vec4(-1, 0, 1, 0),	 glm::vec4(1, 0, -1, 0),   glm::vec4(-1, 0, -1, 0),
				glm::vec4(0, 1, 1, 0),	glm::vec4(0, -1, 1, 0),	 glm::vec4(0, -1, -1, 0),  glm::vec4(0, 1, -1, 0)};
			memcpy(&this->uniform.pcfFilters[0][0], &samples[0][0], sizeof(samples));
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			{
				/*	Draw each point light shadow.	*/
				for (size_t i = 0; i < this->nrPointLights; i++) {

					/*	*/
					glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
									  ((this->getFrameCount() % this->nrUniformBuffer) * this->nrPointLights + i) *
										  this->uniformBufferSize,
									  this->uniformBufferSize);

					glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[i]);

					glClear(GL_DEPTH_BUFFER_BIT);
					glViewport(0, 0, this->shadowWidth, this->shadowHeight);
					glUseProgram(this->shadow_program);

					glCullFace(GL_FRONT);
					glEnable(GL_CULL_FACE);
					/*	Make sure it fills the triangles.	*/
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

					/*	Setup the shadow.	*/
					// TODO enbable.
					glVertexAttribI1i(4, i);
					this->scene.render();

					glBindFramebuffer(GL_FRAMEBUFFER, 0);
				}
			}
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  ((this->getFrameCount() % this->nrUniformBuffer) * this->nrPointLights) *
									  this->uniformBufferSize,
								  this->uniformBufferSize);
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

				if (this->shadowSettingComponent->use_pcf) {
					glUseProgram(this->graphic_pfc_program);
				} else {
					glUseProgram(this->graphic_program);
				}

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->shadowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				for (size_t j = 0; j < this->nrPointLights; j++) {
					glActiveTexture(GL_TEXTURE2 + j);
					glBindTexture(GL_TEXTURE_CUBE_MAP, this->pointShadowTextures[j]);
				}

				this->scene.render();
			}
		}

		void update() override {

			/*	Update Camera.	*/
			this->scene.update(this->getTimer().deltaTime());
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			if (this->shadowSettingComponent->animate) {
				for (size_t i = 0; i < this->nrPointLights; i++) {
					this->uniform.pointLights[i].position =
						glm::vec3(5.0f * std::cos(this->getTimer().getElapsed() * 0.51415 + 1.3 * i), 10,
								  5.0f * std::sin(this->getTimer().getElapsed() * 0.51415 + 1.3 * i));
				}
			}

			/*	*/
			this->uniform.proj = this->camera.getProjectionMatrix();

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
				GL_UNIFORM_BUFFER,
				(((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->nrPointLights) * this->uniformBufferSize,
				this->uniformBufferSize * this->nrPointLights, GL_MAP_WRITE_BIT);
			glm::mat4 PointView[6];

			/*	*/
			for (size_t i = 0; i < this->nrPointLights; i++) {
				const glm::vec3 lightPosition = this->uniform.pointLights[i].position;
				/*	*/
				glm::mat4 model = glm::mat4(1.0f);
				glm::mat4 translation = glm::translate(model, this->uniform.pointLights[i].position);

				PointView[0] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
				PointView[1] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
				PointView[2] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
				PointView[3] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
				PointView[4] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
				PointView[5] =
					glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

				/*	Compute light matrices.	*/
				glm::mat4 pointPer =
					glm::perspective(glm::radians(90.0f), (float)this->shadowWidth / (float)this->shadowHeight, 0.15f,
									 this->uniform.pointLights[i].range);

				for (size_t j = 0; j < 6; j++) {

					this->uniform.ViewProjection[j] = pointPer * PointView[j];
				}

				/*	*/
				this->uniform.model = glm::mat4(1.0f);

				this->uniform.view = this->camera.getViewMatrix();
				this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
				this->uniform.lightPosition = glm::vec4(this->camera.getPosition(), 0.0f);

				memcpy(&uniformPointer[i * this->uniformBufferSize], &this->uniform, sizeof(this->uniform));
			}

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class PointLightShadowGLSample : public GLSample<PointLightShadow> {
	  public:
		PointLightShadowGLSample() : GLSample<PointLightShadow>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PointLightShadowGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}