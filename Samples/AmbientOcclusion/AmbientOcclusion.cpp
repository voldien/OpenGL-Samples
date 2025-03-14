#include "GLUIComponent.h"
#include "Scene.h"
#include "Skybox.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <Math/NormalDistribution.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

namespace glsample {

	/**
	 * Screen Space Ambient Occlusion post processing effect. Both World and Depth Space.
	 * TODO: Downscale.
	 */
	class ScreenSpaceAmbientOcclusion : public GLSampleWindow {
	  public:
		ScreenSpaceAmbientOcclusion() : GLSampleWindow() {
			this->setTitle("ScreenSpace AmbientOcclusion");

			/*	Setting Window.	*/
			this->ambientOcclusionSettingComponent = std::make_shared<AmbientOcclusionSettingComponent>(*this);
			this->addUIComponent(this->ambientOcclusionSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}
		static const int maxKernels = 64;

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};

			/*	Light source.	*/
			DirectionalLight directional_light;

			/*	Material settings.	*/
			glm::vec4 specularColor{};
			glm::vec4 ambientColor{};
			glm::vec4 viewDir{};
			float shininess{};

		} uniformStageBlock;

		struct UniformSSAOBufferBlock {
			glm::mat4 proj{};

			/*	*/
			int samples = 64;
			float radius = 10.5f;
			float intensity = 1.0f;
			float bias = 0.025;

			/*	*/
			glm::vec4 kernel[maxKernels]{};

			glm::vec4 color{};
			glm::vec2 screen{};

		} uniformStageBlockSSAO;

		/*	*/
		MeshObject plan;
		Skybox skybox;
		Scene scene;

		/*	G-Buffer	*/
		unsigned int multipass_framebuffer{};
		unsigned int multipass_texture_width{};
		unsigned int multipass_texture_height{};
		std::vector<unsigned int> multipass_textures;
		unsigned int world_position_sampler = 0;
		unsigned int depthTexture{};

		unsigned int ssao_framebuffer{};
		unsigned int ssaoTexture{};
		unsigned int ssao_down_sample_index = 2;

		/*	White texture for each object.	*/
		unsigned int white_texture{};
		/*	Random direction texture.	*/
		unsigned int random_texture{};

		/*	*/
		unsigned int multipass_program{};
		unsigned int ssao_world_program{};
		unsigned int ssao_depth_program{};
		unsigned int texture_program{};

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_ssao_buffer_binding = 1;
		unsigned int uniform_buffer{};
		unsigned int uniform_ssao_buffer{};
		const size_t nrUniformBuffer = 3;

		/*	*/
		size_t uniformBufferAlignedSize = sizeof(uniform_buffer_block);
		size_t uniformSSAOBufferAlignSize = sizeof(UniformSSAOBufferBlock);

		CameraController camera;

		class AmbientOcclusionSettingComponent : public GLUIComponent<ScreenSpaceAmbientOcclusion> {
		  public:
			AmbientOcclusionSettingComponent(ScreenSpaceAmbientOcclusion &base)
				: GLUIComponent<ScreenSpaceAmbientOcclusion>(base, "Ambient Occlusion Settings"),
				  uniform_ssao(getRefSample().uniformStageBlockSSAO) {}

			void draw() override {

				ImGui::TextUnformatted("Ambient Occlusion Settings");
				ImGui::DragFloat("Intensity", &uniform_ssao.intensity, 0.1f, 0.0f);
				ImGui::DragFloat("Radius", &uniform_ssao.radius, 0.35f, 0.0f);
				ImGui::DragInt("Sample", &uniform_ssao.samples, 1, 0);
				ImGui::DragFloat("Bias", &uniform_ssao.bias, 0.01f, 0, 1);
				ImGui::Checkbox("DownSample", &this->downScale);
				ImGui::Checkbox("Use Depth Only", &this->useDepthOnly);

				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->getRefSample().uniformStageBlock.directional_light.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction",
								  &this->getRefSample().uniformStageBlock.directional_light.lightDirection[0]);

				ImGui::TextUnformatted("Debugging");
				ImGui::Checkbox("Show Only AO", &this->showAOOnly);
				ImGui::Checkbox("Show GBuffer", &this->showGBuffers);
				ImGui::Checkbox("Show Wireframe", &this->showWireframe);
				ImGui::Checkbox("Use AO", &this->useAO);

				this->drawCameraController(this->getRefSample().camera);
			}

			bool showWireframe = false;
			bool downScale = false;
			bool useDepthOnly = false;
			bool showAOOnly = true;
			bool showGBuffers = false;
			bool useAO = true;

		  private:
			struct UniformSSAOBufferBlock &uniform_ssao;
		};
		std::shared_ptr<AmbientOcclusionSettingComponent> ambientOcclusionSettingComponent;

		/*	Shader to extract values.	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		/*	*/
		const std::string vertexSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
		const std::string fragmentSSAOShaderPath = "Shaders/ambientocclusion/ambientocclusion.frag.spv";

		/*	*/
		const std::string vertexSSAODepthOnlyShaderPath = "Shaders/ambientocclusion/ambientocclusion.vert.spv";
		const std::string fragmentSSAODepthOnlyShaderPath =
			"Shaders/ambientocclusion/ambientocclusion_depthonly.frag.spv";

		/*	*/
		const std::string vertexOverlayShaderPath = "Shaders/postprocessingeffects/overlay.vert.spv";
		const std::string fragmentOverlayTextureShaderPath = "Shaders/postprocessingeffects/overlay.frag.spv";

		void Release() override {
			this->scene.release();

			/*	Delete graphic pipelines.	*/
			glDeleteProgram(this->ssao_world_program);
			glDeleteProgram(this->ssao_depth_program);
			glDeleteProgram(this->multipass_program);
			glDeleteProgram(this->texture_program);

			/*	*/
			glDeleteFramebuffers(1, &this->multipass_framebuffer);
			glDeleteFramebuffers(1, &this->ssao_framebuffer);

			/*	Delete textures.	*/
			glDeleteTextures(1, &this->depthTexture);
			glDeleteTextures(this->multipass_textures.size(), this->multipass_textures.data());
			glDeleteTextures(1, &this->random_texture);
			glDeleteTextures(1, &this->white_texture);
			glDeleteTextures(1, &this->ssaoTexture);

			/*	Delete uniform buffer.	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	Delete geometry data.	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_ssao_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSSAOShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_ssao_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSSAOShaderPath, this->getFileSystem());

				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_ssao_depth_only_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSSAODepthOnlyShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_ssao_depth_onlysource =
					IOUtil::readFileData<uint32_t>(this->fragmentSSAODepthOnlyShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_multi_pass_binary =
					IOUtil::readFileData<uint32_t>(this->vertexMultiPassShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_multi_pass_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentMultiPassShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> texture_vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexOverlayShaderPath, this->getFileSystem());
				const std::vector<uint32_t> texture_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentOverlayTextureShaderPath, this->getFileSystem());

				/*	Load shader	*/
				this->ssao_world_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_ssao_binary, &fragment_ssao_binary);

				/*	Load shader	*/
				this->ssao_depth_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_ssao_depth_only_binary, &fragment_ssao_depth_onlysource);

				/*	Load shader	*/
				this->multipass_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_multi_pass_binary,
																		   &fragment_multi_pass_binary);

				/*	Load shader	*/
				this->texture_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &texture_vertex_binary, &texture_fragment_binary);
			}

			/*	Setup graphic ambient occlusion pipeline.	*/
			glUseProgram(this->ssao_world_program);
			int uniform_ssao_world_buffer_index =
				glGetUniformBlockIndex(this->ssao_world_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "WorldTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "NormalTexture"), 3);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "DepthTexture"), 4);
			glUniform1iARB(glGetUniformLocation(this->ssao_world_program, "NormalRandomize"), 5);
			glUniformBlockBinding(this->ssao_world_program, uniform_ssao_world_buffer_index,
								  this->uniform_ssao_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic ambient occlusion pipeline.	*/
			glUseProgram(this->ssao_depth_program);
			int uniform_ssao_depth_buffer_index =
				glGetUniformBlockIndex(this->ssao_depth_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->ssao_depth_program, "WorldTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->ssao_depth_program, "NormalTexture"), 3);
			glUniform1iARB(glGetUniformLocation(this->ssao_depth_program, "DepthTexture"), 4);
			glUniform1iARB(glGetUniformLocation(this->ssao_depth_program, "NormalRandomize"), 5);
			glUniformBlockBinding(this->ssao_depth_program, uniform_ssao_depth_buffer_index,
								  this->uniform_ssao_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic multipass pipeline.	*/
			glUseProgram(this->multipass_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->multipass_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "DiffuseTexture"), 0);
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "NormalTexture"), 1);
			glUniform1iARB(glGetUniformLocation(this->multipass_program, "AlphaMaskedTexture"), 2);

			glUniformBlockBinding(this->multipass_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic program.	*/
			glUseProgram(this->texture_program);
			glUniform1i(glGetUniformLocation(this->texture_program, "ColorTexture"), 0);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferAlignedSize =
				fragcore::Math::align<size_t>(this->uniformBufferAlignedSize, (size_t)minMapBufferSize);
			this->uniformSSAOBufferAlignSize =
				fragcore::Math::align<size_t>(this->uniformSSAOBufferAlignSize, (size_t)minMapBufferSize);

			/*	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferAlignedSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			glGenBuffers(1, &this->uniform_ssao_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSSAOBufferAlignSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
			skybox.Init(skytexture, Skybox::loadDefaultProgram(this->getFileSystem()));

			this->scene = Scene::loadFrom(modelLoader);

			/*	Load geometry.	*/
			Common::loadPlan(this->plan, 1, 1, 1);

			/*	FIXME: improve vectors.		*/
			{
				/*	Create random vector.	*/
				std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
				std::default_random_engine generator;
				for (size_t i = 0; i < maxKernels; ++i) {

					glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
									 randomFloats(generator));

					sample = glm::normalize(sample);
					sample *= randomFloats(generator);

					float scale = (float)i / static_cast<float>(maxKernels);
					scale = fragcore::Math::lerp(0.1f, 1.0f, scale * scale);

					sample *= scale;
					this->uniformStageBlockSSAO.kernel[i] = glm::vec4(sample, 0);
				}

				/*	Create white texture.	*/
				this->white_texture = Common::createColorTexture(1, 1, Color::white());

				/*	Create noise normalMap.	*/
				const size_t noiseW = 4;
				const size_t noiseH = 4;
				std::vector<glm::vec3> ssaoRandomNoise(noiseW * noiseH);
				for (size_t i = 0; i < ssaoRandomNoise.size(); i++) {
					ssaoRandomNoise[i].r = randomFloats(generator) * 2.0 - 1.0;
					ssaoRandomNoise[i].g = randomFloats(generator) * 2.0 - 1.0;
					ssaoRandomNoise[i].b = 0.0f;
					/*	*/
					ssaoRandomNoise[i] = glm::normalize(ssaoRandomNoise[i]);
				}

				/*	Create random texture.	*/
				glGenTextures(1, &this->random_texture);
				glBindTexture(GL_TEXTURE_2D, this->random_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, noiseW, noiseH, 0, GL_RGB, GL_FLOAT, ssaoRandomNoise.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

				FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Create sampler.	*/
			glCreateSamplers(1, &this->world_position_sampler);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glSamplerParameterf(this->world_position_sampler, GL_TEXTURE_LOD_BIAS, 0.0f);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MAX_LOD, 0);
			glSamplerParameteri(this->world_position_sampler, GL_TEXTURE_MIN_LOD, 0);

			/*	Create multipass framebuffer.	*/
			{

				glGenFramebuffers(1, &this->multipass_framebuffer);
				glGenFramebuffers(1, &this->ssao_framebuffer);

				/*	Setup framebuffer textures.	*/
				this->multipass_textures.resize(4);
				glGenTextures(this->multipass_textures.size(), this->multipass_textures.data());
				glGenTextures(1, &this->depthTexture);
				glGenTextures(1, &this->ssaoTexture);

				this->onResize(this->width(), this->height());
			}

			this->camera.setNear(0.15f);
			this->camera.setFar(1500.0f);
		}

		void onResize(int width, int height) override {

			this->multipass_texture_width = width;
			this->multipass_texture_height = height;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->multipass_framebuffer);

			/*	Resize the image.	*/
			std::vector<GLenum> drawAttach(multipass_textures.size());
			for (size_t i = 0; i < multipass_textures.size(); i++) {

				/*	*/
				glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->multipass_texture_width,
							 this->multipass_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

				FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
				glBindTexture(GL_TEXTURE_2D, 0);

				/*	*/
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->multipass_textures[i], 0);
				drawAttach[i] = GL_COLOR_ATTACHMENT0 + i;
			}

			/*	Create depth buffer texture.	*/
			glBindTexture(GL_TEXTURE_2D, this->depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, this->multipass_texture_width,
						 this->multipass_texture_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

			FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

			FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));

			/*	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/*	Border clamped to max value, it makes the outside area.	*/
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

			glDrawBuffers(drawAttach.size(), drawAttach.data());

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ssao_framebuffer);

				glBindTexture(GL_TEXTURE_2D, this->ssaoTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, this->multipass_texture_width, this->multipass_texture_height, 0,
							 GL_RED, GL_FLOAT, nullptr);

				GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
				glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0));
				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 4));
				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4));
				FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));
				FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));

				glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, 0);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->ssaoTexture, 0);

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			/*	Update camera aspect.	*/
			this->camera.setAspect((float)width / (float)height);
			this->camera.setFar(1500.0f);
			this->camera.setNear(0.5f);
		}

		void draw() override {

			this->update();
			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			this->uniformStageBlockSSAO.screen = glm::vec2(width, height);

			/*	G-Buffer extraction.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferAlignedSize,
								  this->uniformBufferAlignedSize);

				glBindFramebuffer(GL_FRAMEBUFFER, this->multipass_framebuffer);
				glViewport(0, 0, this->multipass_texture_width, this->multipass_texture_height);
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				/*	*/
				glUseProgram(this->multipass_program);

				glDisable(GL_CULL_FACE);

				this->scene.render();

				glUseProgram(0);

				skybox.Render(this->camera);
			}

			/*	Draw post processing effect - Screen Space Ambient Occlusion.	*/
			if (this->ambientOcclusionSettingComponent->useAO) {

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer_binding, this->uniform_ssao_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSSAOBufferAlignSize,
								  this->uniformSSAOBufferAlignSize);

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ssao_framebuffer);

				glViewport(0, 0, width, height);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_DEPTH_BUFFER_BIT);

				if (this->ambientOcclusionSettingComponent->useDepthOnly) {
					glUseProgram(this->ssao_depth_program);
				} else {
					glUseProgram(this->ssao_world_program);
				}

				glDisable(GL_CULL_FACE);

				for (size_t i = 0; i < this->multipass_textures.size(); i++) {
					/*	*/
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, this->multipass_textures[i]);
					glBindSampler(i, this->world_position_sampler);
				}
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, this->depthTexture);
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, this->random_texture);

				/*	Draw.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);

				/*	Downscale for creating a cheap blurred version, using the pixel bilinear filtering for each power
				 * of 2.	*/
				// TODO fix downscale.
				if (this->ambientOcclusionSettingComponent->downScale) {
					/*	Downscale the image.	*/
					glBindFramebuffer(GL_READ_FRAMEBUFFER, this->ssao_framebuffer);
					glReadBuffer(GL_COLOR_ATTACHMENT0);

					// TODO: check if a better way for downsampling.
					glBindTexture(GL_TEXTURE_2D, this->ssaoTexture);
					glGenerateMipmap(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}

			/*	Render final result.	*/
			{

				/*	Show only ambient Occlusion.	*/
				if (this->ambientOcclusionSettingComponent->showAOOnly) {
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
					glBindFramebuffer(GL_READ_FRAMEBUFFER, this->ssao_framebuffer);
					glViewport(0, 0, width, height);

					// glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->ssaoTexture,
					// ssao_down_sample_index);

					glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
									  height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

					// glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->ssaoTexture,
					// 0);

				} else { /*	Blend with final result.	*/

					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
					glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);
					glReadBuffer(GL_COLOR_ATTACHMENT0);

					glViewport(0, 0, width, height);

					glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height, 0, 0, width,
									  height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

					glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
					glDisable(GL_CULL_FACE);
					glCullFace(GL_FRONT_AND_BACK);
					glDisable(GL_DEPTH_TEST);

					/* graphic pipeline. - texture	*/
					glUseProgram(this->texture_program);

					/*	Draw.	*/
					glBindVertexArray(this->plan.vao);

					glEnable(GL_BLEND);
					glBlendEquation(GL_FUNC_ADD);
					glBlendFunc(GL_DST_COLOR, GL_ZERO);

					// Blend to color layer.
					if (this->ambientOcclusionSettingComponent->useAO) {
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, this->ssaoTexture);

						if (this->ambientOcclusionSettingComponent->downScale) {
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, this->ssao_down_sample_index);
						}

						/*	*/
						glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
					glBindVertexArray(0);

					glUseProgram(0);
				}
			}

			if (this->ambientOcclusionSettingComponent->showGBuffers) {

				/*	Blit image targets to screen.	*/
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multipass_framebuffer);

				const size_t widthDivior = 2;
				const size_t heightDivior = 2;

				/*	Transfer each target to default framebuffer.	*/
				const float halfW = (width / widthDivior) * 0.5f;
				const float halfH = (height / heightDivior) * 0.5f;

				for (size_t i = 0; i < this->multipass_textures.size(); i++) {
					glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
					glBlitFramebuffer(0, 0, this->multipass_texture_width, this->multipass_texture_height,
									  (i % 2) * (halfW), (i / 2) * halfH, halfW + (i % 2) * halfW,
									  halfH + (i / 2) * halfH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
		}

		void update() override {

			/*	Update Camera and Scene.	*/
			this->camera.update(this->getTimer().deltaTime<float>());
			this->scene.update(this->getTimer().deltaTime<float>());

			this->uniformStageBlock.proj = this->camera.getProjectionMatrix();
			this->uniformStageBlockSSAO.proj = this->camera.getProjectionMatrix();
			/*	*/
			{
				this->uniformStageBlock.model = glm::mat4(1.0f);
				this->uniformStageBlock.model = glm::scale(this->uniformStageBlock.model, glm::vec3(1.f));
				this->uniformStageBlock.view = this->camera.getViewMatrix();
				this->uniformStageBlock.modelView = (this->uniformStageBlock.view * this->uniformStageBlock.model);
				this->uniformStageBlock.modelViewProjection =
					this->uniformStageBlock.proj * this->uniformStageBlock.view * this->uniformStageBlock.model;
			}

			/*	Update uniform buffers.	*/
			{
				glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferAlignedSize,
					this->uniformBufferAlignedSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformStageBlock, sizeof(uniformStageBlock));
				glUnmapBufferARB(GL_UNIFORM_BUFFER);

				/*	*/
				glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_ssao_buffer);
				void *uniformSSAOPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSSAOBufferAlignSize,
					this->uniformSSAOBufferAlignSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformSSAOPointer, &this->uniformStageBlockSSAO, sizeof(uniformStageBlockSSAO));
				glUnmapBufferARB(GL_UNIFORM_BUFFER);
			}
		}
	};

	/*	*/
	class AmbientOcclusionGLSample : public GLSample<ScreenSpaceAmbientOcclusion> {
	  public:
		AmbientOcclusionGLSample() : GLSample<ScreenSpaceAmbientOcclusion>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::AmbientOcclusionGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}