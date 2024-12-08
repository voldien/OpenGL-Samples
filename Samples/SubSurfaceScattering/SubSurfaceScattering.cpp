#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class SubSurfaceScattering : public GLSampleWindow {
	  public:
		SubSurfaceScattering() : GLSampleWindow() {
			this->setTitle("SubSurfaceScattering");
			this->subSurfaceScatteringSettingComponent =
				std::make_shared<BasicShadowMapSettingComponent>(this->uniform, this->shadowTexture);
			this->addUIComponent(this->subSurfaceScatteringSettingComponent);
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;
			glm::mat4 lightModelProject;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(0.5f, 0.5f, 0.6f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.05, 0.05, 0.05, 1.0f);
			glm::vec4 cameraPosition;
			glm::vec4 subsurfaceColor = glm::vec4(0.095, 0.012f, 0.012f, 1.0f);

			/*	Settings.	*/
			float bias = 0.01f;
			float shadowStrength = 1.0f;
			float sigma = 9.0f;
			float range = 80.0f;
		} uniform;

		/*	*/
		unsigned int shadowFramebuffer;
		unsigned int shadowTexture;
		size_t shadowWidth = 4096;
		size_t shadowHeight = 4096;

		unsigned int diffuse_texture;

		std::vector<MeshObject> refObj;

		/*	*/
		unsigned int graphic_subsurface_scattering_program;
		unsigned int shadow_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class BasicShadowMapSettingComponent : public nekomimi::UIComponent {
		  public:
			BasicShadowMapSettingComponent(struct uniform_buffer_block &uniform, unsigned int &depth)
				: depth(depth), uniform(uniform) {
				this->setName("SubSurface Scattering Settings");
			}
			void draw() override {

				ImGui::TextUnformatted("Light Settings");

				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("SubSurface Color", &this->uniform.subsurfaceColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Shadow Settings");

				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);

				ImGui::TextUnformatted("SubSurface Settings");
				ImGui::DragFloat("Distance", &this->uniform.range);
				ImGui::DragFloat("Sigma", &this->uniform.sigma);

				ImGui::TextUnformatted("Debugging");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");
				ImGui::Image(static_cast<ImTextureID>(this->depth), ImVec2(512, 512));
			}

			unsigned int &depth;
			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<BasicShadowMapSettingComponent> subSurfaceScatteringSettingComponent;

		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/subsurfacescattering/subsurfacescattering.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/subsurfacescattering/subsurfacescattering.frag.spv";

		const std::string vertexShadowShaderPath = "Shaders/shadowmap/shadowmap.vert.spv";
		const std::string fragmentShadowShaderPath = "Shaders/shadowmap/shadowmap.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_subsurface_scattering_program);
			glDeleteProgram(this->shadow_program);

			glDeleteFramebuffers(1, &this->shadowFramebuffer);
			glDeleteTextures(1, &this->shadowTexture);

			glDeleteBuffers(1, &this->uniform_buffer);

			glDeleteVertexArrays(1, &this->refObj[0].vao);
			glDeleteBuffers(1, &this->refObj[0].vbo);
			glDeleteBuffers(1, &this->refObj[0].ibo);
		}

		void Initialize() override {
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string diffuseTexturePath = "asset/diffuse.png";

			{
				/*	*/
				std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGraphicShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGraphicShaderPath, this->getFileSystem());

				/*	*/
				std::vector<uint32_t> vertex_shadow_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShadowShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_shadow_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShadowShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shaders	*/
				this->graphic_subsurface_scattering_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
				this->shadow_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_shadow_binary, &fragment_shadow_binary);
			}

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath, ColorSpace::SRGB);

			/*	*/
			glUseProgram(this->shadow_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shadow_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_subsurface_scattering_program);
			uniform_buffer_index =
				glGetUniformBlockIndex(this->graphic_subsurface_scattering_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_subsurface_scattering_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_subsurface_scattering_program, "ShadowTexture"), 1);
			glUniformBlockBinding(this->graphic_subsurface_scattering_program, uniform_buffer_index,
								  this->uniform_buffer_binding);
			glUseProgram(0);

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
				glGenFramebuffers(1, &this->shadowFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, this->shadowFramebuffer);

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

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->shadowTexture, 0);
				int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {

					/*  Delete  */
					glDeleteFramebuffers(1, &shadowFramebuffer);
					throw RuntimeException("Failed to create framebuffer, {}",
										   (const char *)glewGetErrorString(frstat));
				}
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			ImportHelper::loadModelBuffer(modelLoader, refObj);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {
			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	Compute light matrices.	*/
			const float near_plane = -this->uniform.range / 2.0f, far_plane = this->uniform.range / 2.0f;
			glm::mat4 lightProjection = glm::ortho(near_plane, far_plane, near_plane, far_plane, near_plane, far_plane);
			glm::mat4 lightView =
				glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f),
							glm::vec3(this->uniform.direction.x, this->uniform.direction.y, this->uniform.direction.z),
							glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
			this->uniform.lightModelProject = lightSpaceMatrix;

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
							  this->uniformAlignBufferSize);

			/*	Render from light perspective.	*/
			{

				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->shadowFramebuffer);

				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, this->shadowWidth, this->shadowHeight);
				glUseProgram(this->shadow_program);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				/*	*/
				glCullFace(GL_BACK);
				/*	*/
				glEnable(GL_CULL_FACE);

				/*	Setup the shadow.	*/
				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	*/
			glClearColor(0.05f, 0.05, 0.05, 0.0f);
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glUseProgram(this->graphic_subsurface_scattering_program);

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK,
							  this->subSurfaceScatteringSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Texture.	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Shadow texture.	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->shadowTexture);

				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
			}
		}

		void update() override {
			/*	Update Camera.	*/
			camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			this->uniform.model = glm::scale(this->uniform.model, glm::vec3(20));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.proj = this->camera.getProjectionMatrix();

			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.cameraPosition = glm::vec4(this->camera.getPosition(), 0.0f);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(this->uniform));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class SubSurfaceScatteringGLSample : public GLSample<SubSurfaceScattering> {
	  public:
		SubSurfaceScatteringGLSample() : GLSample<SubSurfaceScattering>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"))(
				"T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SubSurfaceScatteringGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}