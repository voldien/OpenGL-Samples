#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class VarianceShadow : public GLSampleWindow {
	  public:
		VarianceShadow() : GLSampleWindow() {
			this->setTitle("VarianceShadow");

			this->shadowSettingComponent =
				std::make_shared<VarianceShadowSettingComponent>(this->uniform, this->shadowTexture);
			this->addUIComponent(this->shadowSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));
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
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1.0f);
			glm::vec3 cameraPosition;

			float bias = 0.01f;
			float shadowStrength = 1.0f;
		} uniform;

		unsigned int shadowFramebuffer;
		unsigned int shadowTexture;
		size_t shadowWidth = 4096;
		size_t shadowHeight = 4096;

		std::string diffuseTexturePath = "asset/diffuse.png";

		unsigned int diffuse_texture;

		std::vector<GeometryObject> refObj;

		unsigned int graphic_program;
		unsigned int shadow_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class VarianceShadowSettingComponent : public nekomimi::UIComponent {
		  public:
			VarianceShadowSettingComponent(struct uniform_buffer_block &uniform, unsigned int &depth)
				: uniform(uniform), depth(depth) {
				this->setName("Basic Shadow Mapping Settings");
			}
			void draw()  {

				ImGui::DragFloat("Shadow Strength", &this->uniform.shadowStrength, 1, 0.0f, 1.0f);
				ImGui::DragFloat("Shadow Bias", &this->uniform.bias, 1, 0.0f, 1.0f);
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::DragFloat("Distance", &this->distance);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");
				ImGui::Image((ImTextureID)this->depth, ImVec2(512, 512));
			}

			float distance = 50.0;
			unsigned int &depth;
			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<BasicShadowMapSettingComponent> shadowSettingComponent;

		const std::string modelPath = "asset/sponza/sponza.obj";
		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/shadowmap/texture.vert.spv";
		const std::string fragmentGraphicShaderPath = "Shaders/shadowmap/texture.frag.spv";

		const std::string vertexShadowShaderPath = "Shaders/shadowmap/shadowmap.vert.spv";
		const std::string fragmentShadowShaderPath = "Shaders/shadowmap/shadowmap.frag.spv";

		void Release()  {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->shadow_program);

			glDeleteFramebuffers(1, &this->shadowFramebuffer);
			glDeleteTextures(1, &this->shadowTexture);

			glDeleteBuffers(1, &this->uniform_buffer);

			glDeleteVertexArrays(1, &this->refObj[0].vao);
			glDeleteBuffers(1, &this->refObj[0].vbo);
			glDeleteBuffers(1, &this->refObj[0].ibo);
		}

		void Initialize()  {

			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

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

				std::vector<char> vertex_binary_T =
					fragcore::ShaderCompiler::convertSPIRV(vertex_binary, compilerOptions);
				std::vector<char> fragment_binary_T =
					fragcore::ShaderCompiler::convertSPIRV(fragment_binary, compilerOptions);

				std::vector<char> vertex_shadow_binary_T =
					fragcore::ShaderCompiler::convertSPIRV(vertex_shadow_binary, compilerOptions);
				std::vector<char> fragment_shadow_binary_T =
					fragcore::ShaderCompiler::convertSPIRV(fragment_shadow_binary, compilerOptions);

				/*	Load shaders	*/
				this->graphic_program = ShaderLoader::loadGraphicProgram(&vertex_binary_T, &fragment_binary_T);
				this->shadow_program =
					ShaderLoader::loadGraphicProgram(&vertex_shadow_binary_T, &fragment_shadow_binary_T);
			}

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	*/
			glUseProgram(this->shadow_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->shadow_program, "UniformBufferBlock");
			glUniformBlockBinding(this->shadow_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 1);
			glUniformBlockBinding(this->graphic_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				glGenFramebuffers(1, &shadowFramebuffer);
				glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

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
			ModelImporter modelLoader = new ModelImporter(this->getFileSystem());
			modelLoader->loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(*modelLoader);

			ImportHelper::loadModelBuffer(modelLoader, refObj);
		}

		void onResize(int width, int height)  { this->camera.setAspect((float)width / (float)height); }

		void draw()  {

			int width, height;
			getSize(&width, &height);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize, this->uniformAlignBufferSize);

			{

				/*	Compute light matrices.	*/
				float near_plane = -this->shadowSettingComponent->distance / 2.0f,
					  far_plane = this->shadowSettingComponent->distance / 2.0f;
				glm::mat4 lightProjection =
					glm::ortho(-this->shadowSettingComponent->distance, this->shadowSettingComponent->distance,
							   -this->shadowSettingComponent->distance, this->shadowSettingComponent->distance,
							   near_plane, far_plane);
				glm::mat4 lightView = glm::lookAt(
					glm::vec3(-2.0f, 4.0f, -1.0f),
					glm::vec3(this->uniform.direction.x, this->uniform.direction.y, this->uniform.direction.z),
					glm::vec3(0.0f, 1.0f, 0.0f));
				glm::mat4 lightSpaceMatrix = lightProjection * lightView;

				glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
				this->uniform.lightModelProject = lightSpaceMatrix;

				glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

				glClear(GL_DEPTH_BUFFER_BIT);
				glViewport(0, 0, shadowWidth, shadowHeight);
				glUseProgram(this->shadow_program);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);

				/*	Setup the shadow.	*/

				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}

			{
				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glUseProgram(this->graphic_program);

				glCullFace(GL_BACK);
				// glDisable(GL_CULL_FACE);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, shadowSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
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

		void update()  {
			/*	Update Camera.	*/
			camera.update(getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			// this->mvp.model = glm::scale(this->mvp.model, glm::vec3(1.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;
			this->uniform.cameraPosition = this->camera.getPosition();

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(uniform));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class VarianceShadowGLSample : public GLSample<VarianceShadow> {
	  public:
		VarianceShadowGLSample(int argc, const char **argv) : GLSample<VarianceShadow>(argc, argv) {}
		virtual void commandline(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::VarianceShadowGLSample sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}