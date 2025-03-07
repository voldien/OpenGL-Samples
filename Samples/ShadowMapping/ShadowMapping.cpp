#include "SampleHelper.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <Importer/Scene.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ShadowMapping : public GLSampleWindow {
	  public:
		ShadowMapping() : GLSampleWindow() {
			this->setTitle("ShadowMapping");
			this->shadowSettingComponent = std::make_shared<BasicShadowMapSettingComponent>(*this);
			this->addUIComponent(this->shadowSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct alignas(16) uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;
			glm::mat4 lightModelProject;

			/*	light source.	*/
			DirectionalLight directional;
			CameraInstance camera;

			/*	*/
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1.0f);
			glm::vec4 diffuseColor = glm::vec4(1, 1, 1, 1.0f);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1.0f);

			float bias = 0.01f;
			float shadowStrength = 1.0f;
			float pcfRadius = 1.0f;
		} uniformStageBuffer;

		/*	*/
		unsigned int shadowFramebuffer;
		unsigned int shadowTexture;
		size_t shadowWidth = static_cast<long>(4096) * 2;
		size_t shadowHeight = static_cast<long>(4096) * 2;

		Scene scene;
		Skybox skybox;

		unsigned int irradiance_texture;

		/*	*/
		unsigned int graphic_program;
		unsigned int graphic_pfc_program;
		unsigned int shadow_program;
		unsigned int shadow_alpha_clip_program;
		unsigned int skybox_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_shadow_buffer_binding = 0;
		unsigned int uniform_graphic_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		const int shadowBinding = 15;

		CameraController camera;

		class BasicShadowMapSettingComponent : public GLUIComponent<ShadowMapping> {
		  public:
			BasicShadowMapSettingComponent(ShadowMapping &sample)
				: GLUIComponent(sample, "Basic Shadow Mapping Settings"), depth(this->getRefSample().shadowTexture),
				  uniform(this->getRefSample().uniformStageBuffer) {}

			void draw() override {

				ImGui::DragFloat("Shadow Strength", &this->uniform.shadowStrength, 1, 0.0f, 1.0f);
				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);
				ImGui::DragFloat("PCF Radius", &this->uniform.pcfRadius, 1, 0.0f, 100.0f);
				ImGui::ColorEdit4("Light", &this->uniform.directional.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.directional.lightDirection[0]);

				ImGui::TextUnformatted("Material Settings");
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Diffuse", &this->uniform.diffuseColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
				ImGui::DragFloat("Shininess", &this->uniform.specularColor[3]);

				ImGui::DragFloat("Distance", &this->distance);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("PCF Shadow", &this->use_pcf);
				ImGui::Checkbox("Shadow Alpha Clipping", &this->useShadowClip);
				ImGui::TextUnformatted("Depth Texture");
				ImGui::Image(static_cast<ImTextureID>(this->depth), ImVec2(512, 512), ImVec2(1, 1), ImVec2(0, 0));

				this->getRefSample().scene.renderUI();
			}

			float distance = 50.0;
			unsigned int &depth;
			bool showWireFrame = false;
			bool use_pcf = false;
			bool useShadowClip = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<BasicShadowMapSettingComponent> shadowSettingComponent;

		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/shadowmap/texture.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/shadowmap/texture.frag.spv";
		const std::string fragmentPCFGraphicShaderPath = "Shaders/shadowmap/texture_pcf.frag.spv";

		/*	*/
		const std::string vertexShadowShaderPath = "Shaders/shadowmap/shadowmap.vert.spv";
		const std::string fragmentShadowShaderPath = "Shaders/shadowmap/shadowmap.frag.spv";
		const std::string fragmentClippingShadowShaderPath = "Shaders/shadowmap/shadowmap_alpha.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->graphic_pfc_program);
			glDeleteProgram(this->shadow_program);

			glDeleteFramebuffers(1, &this->shadowFramebuffer);
			glDeleteTextures(1, &this->shadowTexture);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_graphic_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_graphic_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_graphic_pfc_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentPCFGraphicShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> vertex_shadow_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_shadow_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_shadow_alpha_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentClippingShadowShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shaders	*/
				this->graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_binary);
				this->graphic_pfc_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary,
																			 &fragment_graphic_pfc_binary);
				this->shadow_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_shadow_binary, &fragment_shadow_binary);
				this->shadow_alpha_clip_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_shadow_binary, &fragment_shadow_alpha_binary);

				this->skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());
			}

			{
				/*	Setup shadow graphic pipeline.	*/
				glUseProgram(this->shadow_program);
				int uniform_buffer_shadow_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
				glUniformBlockBinding(this->shadow_program, uniform_buffer_shadow_index,
									  this->uniform_shadow_buffer_binding);
				glUseProgram(0);

				glUseProgram(this->shadow_alpha_clip_program);
				glUniform1i(glGetUniformLocation(this->shadow_alpha_clip_program, "DiffuseTexture"),
							TextureType::Diffuse);
				glUniform1i(glGetUniformLocation(this->shadow_alpha_clip_program, "AlphaMaskedTexture"),
							TextureType::AlphaMask);
				uniform_buffer_shadow_index =
					glGetUniformBlockIndex(this->shadow_alpha_clip_program, "UniformBufferBlock");
				glUniformBlockBinding(this->shadow_alpha_clip_program, uniform_buffer_shadow_index,
									  this->uniform_shadow_buffer_binding);
				glUseProgram(0);

				/*	Setup graphic pipeline.	*/
				glUseProgram(this->graphic_program);
				int uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
				glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), TextureType::Diffuse);
				glUniform1i(glGetUniformLocation(this->graphic_program, "AlphaMaskedTexture"), TextureType::AlphaMask);
				glUniform1i(glGetUniformLocation(this->graphic_program, "ShadowTexture"), shadowBinding);
				glUniform1i(glGetUniformLocation(this->graphic_program, "IrradianceTexture"), TextureType::Irradiance);

				glUniformBlockBinding(this->graphic_program, uniform_buffer_index,
									  this->uniform_graphic_buffer_binding);
				glUseProgram(0);

				/*	Setup graphic pipeline.	*/
				glUseProgram(this->graphic_pfc_program);
				uniform_buffer_index = glGetUniformBlockIndex(this->graphic_pfc_program, "UniformBufferBlock");
				glUniform1i(glGetUniformLocation(this->graphic_pfc_program, "DiffuseTexture"), TextureType::Diffuse);
				glUniform1i(glGetUniformLocation(this->graphic_pfc_program, "AlphaMaskedTexture"),
							TextureType::AlphaMask);
				glUniform1i(glGetUniformLocation(this->graphic_pfc_program, "ShadowTexture"), shadowBinding);
				glUniform1i(glGetUniformLocation(this->graphic_program, "IrradianceTexture"), TextureType::Irradiance);
				glUniformBlockBinding(this->graphic_pfc_program, uniform_buffer_index,
									  this->uniform_graphic_buffer_binding);
				glUseProgram(0);
			}

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

			{
				/*	Create shadow map.	*/
				glGenFramebuffers(1, &shadowFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

				/*	Clamp texture size to valid size.	*/
				int max_texture_size = 0;
				glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
				this->shadowWidth = fragcore::Math::min<int>(this->shadowWidth, max_texture_size);
				this->shadowHeight = fragcore::Math::min<int>(this->shadowHeight, max_texture_size);

				/*	*/
				glGenTextures(1, &this->shadowTexture);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT,
							 GL_FLOAT, nullptr);
				/*	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
				int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &shadowFramebuffer);
					throw RuntimeException("Failed to create framebuffer, {}",
										   (const char *)glewGetErrorString(frstat));
				}
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			/*	*/
			ModelImporter *modelLoader = new ModelImporter(this->getFileSystem());
			modelLoader->loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(*modelLoader);

			/*	load Skybox Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
			this->skybox.Init(skytexture, this->skybox_program);

			ProcessData util(this->getFileSystem());
			util.computeIrradiance(skytexture, this->irradiance_texture, 256, 128);
		}

		void onResize(int width, int height) override {
			/*	*/
			this->camera.setFar(2000.0f);
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			{

				/*	Compute light matrix.	*/
				const float near_plane = -(this->shadowSettingComponent->distance / 2.0f) * 2;
				const float far_plane = (this->shadowSettingComponent->distance / 2.0f) * 2;
				glm::mat4 lightProjection =
					glm::ortho(-this->shadowSettingComponent->distance, this->shadowSettingComponent->distance,
							   -this->shadowSettingComponent->distance, this->shadowSettingComponent->distance,
							   near_plane, far_plane);
				glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f),
												  glm::vec3(this->uniformStageBuffer.directional.lightDirection.x,
															this->uniformStageBuffer.directional.lightDirection.y,
															this->uniformStageBuffer.directional.lightDirection.z),
												  glm::vec3(0.0f, 1.0f, 0.0f));
				glm::mat4 lightSpaceMatrix = lightProjection * lightView;

				glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
				this->uniformStageBuffer.lightModelProject = lightSpaceMatrix;
				this->uniformStageBuffer.camera = this->camera;

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_shadow_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glBindFramebuffer(GL_FRAMEBUFFER, this->shadowFramebuffer);

				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, shadowWidth, shadowHeight);
				if (this->shadowSettingComponent->useShadowClip) {
					glUseProgram(this->shadow_alpha_clip_program);
				} else {
					glUseProgram(this->shadow_program);
				}

				/*	*/
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);

				/*	Render shadow.	*/
				this->scene.render(nullptr);

				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			/*	Render Graphic.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_graphic_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT);

				/*	*/
				if (this->shadowSettingComponent->use_pcf) {
					glUseProgram(this->graphic_pfc_program);
				} else {
					glUseProgram(this->graphic_program);
				}

				glCullFace(GL_BACK);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + shadowBinding);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);

				glActiveTexture(GL_TEXTURE0 + TextureType::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				this->scene.render(&this->camera);
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			this->scene.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class ShadowMappingGLSample : public GLSample<ShadowMapping> {
	  public:
		ShadowMappingGLSample() : GLSample<ShadowMapping>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {

	try {
		glsample::ShadowMappingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}