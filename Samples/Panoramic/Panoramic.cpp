#include "Scene.h"
#include "Skybox.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Panoramic : public GLSampleWindow {
	  public:
		Panoramic() : GLSampleWindow() {
			this->setTitle("Panoramic View");

			/*	*/
			this->panoramicSettingComponent = std::make_shared<PanoramicSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->panoramicSettingComponent);

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
		} PointLight;

		static const size_t nrCameraFaces = 4;
		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / std::sqrt(2.0f), -1.0f / std::sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 lightPosition;

			PointLight cameraView[nrCameraFaces];
		} uniformStageBuffer;

		/*	Point light shadow maps.	*/
		std::vector<unsigned int> pointShadowFrameBuffers;
		std::vector<unsigned int> cameraFaceTextures;
		std::vector<unsigned int> pointShadowDepthTextures;

		/*	*/
		unsigned int panoramicWidth = 1024;
		unsigned int panoramicHeight = 1024;

		unsigned int diffuse_texture;
		unsigned int reflection_texture;
		MeshObject skybox;
		Scene scene;
		Skybox skybox_;

		/*	*/
		unsigned int graphic_program;	/*	*/
		unsigned int panoramic_program; /*	Show panoramic.	*/
		unsigned int skybox_program;	/*	*/

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class PanoramicSettingComponent : public nekomimi::UIComponent {
		  public:
			PanoramicSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Panoramic Settings");
			}
			void draw() override {

				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");

				/*	*/

				for (size_t i = 0; i < sizeof(uniform.cameraView) / sizeof(uniform.cameraView[0]); i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightvisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Light Color", &this->uniform.cameraView[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Light Position", &this->uniform.cameraView[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.cameraView[i].constant_attenuation);
						ImGui::DragFloat("Light Range", &this->uniform.cameraView[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.cameraView[i].intensity);
					}
					ImGui::PopID();
				}
			}

			bool showWireFrame = false;
			bool animate = false;

			bool lightvisible[4] = {true, true, true, true};

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<PanoramicSettingComponent> panoramicSettingComponent;

		/*	Panoramic shader paths.	*/
		const std::string vertexPanoramicShaderPath = "Shaders/panoramic/panoramic.vert.spv";
		const std::string geomtryPanoramicShaderPath = "Shaders/panoramic/panoramic.frag.spv";
		const std::string fragmentPanoramicShaderPath = "Shaders/panoramic/panoramic.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_program);
			glDeleteProgram(this->panoramic_program);

			glDeleteFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());
			glDeleteTextures(this->cameraFaceTextures.size(), this->cameraFaceTextures.data());

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {
			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_source =
					IOUtil::readFileData<uint32_t>(this->geomtryPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentPanoramicShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->panoramic_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source,
																		   &fragment_source, &geometry_source);

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	*/
			glUseProgram(this->panoramic_program);
			int uniform_buffer_shadow_index = glGetUniformBlockIndex(this->panoramic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->panoramic_program, uniform_buffer_shadow_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->graphic_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->graphic_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->graphic_program, "DiffuseTexture"), 0);
			const int shadows[4] = {1, 2, 3, 4};
			glUniform1iv(glGetUniformLocation(this->graphic_program, "ShadowTexture"), 4, shadows);
			glUniformBlockBinding(this->graphic_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	 Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer * this->nrCameraFaces,
						 nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			{
				/*	Create shadow map.	*/
				this->pointShadowFrameBuffers.resize(this->nrCameraFaces);
				glGenFramebuffers(this->pointShadowFrameBuffers.size(), this->pointShadowFrameBuffers.data());

				this->cameraFaceTextures.resize(this->nrCameraFaces);
				glGenTextures(this->cameraFaceTextures.size(), this->cameraFaceTextures.data());

				for (size_t x = 0; x < pointShadowFrameBuffers.size(); x++) {
					glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[x]);

					/*	*/
					glBindTexture(GL_TEXTURE_CUBE_MAP, this->cameraFaceTextures[x]);

					/*	Setup each face.	*/
					for (size_t i = 0; i < 6; i++) {
						glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, this->panoramicWidth,
									 this->panoramicHeight, 0, GL_RGBA16F, GL_FLOAT, nullptr);
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
					glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->cameraFaceTextures[x], 0);

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

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
			this->skybox_.Init(skytexture, this->skybox_program);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			// ImportHelper::loadModelBuffer(modelLoader, refObj);

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
										glm::vec4(1, 0, 1, 1)};
			for (size_t i = 0; i < this->nrCameraFaces; i++) {
				this->uniformStageBuffer.cameraView[i].range = 25.0f;
				this->uniformStageBuffer.cameraView[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(2.0f);
				this->uniformStageBuffer.cameraView[i].color = colors[i];
				this->uniformStageBuffer.cameraView[i].constant_attenuation = 1.7f;
				this->uniformStageBuffer.cameraView[i].linear_attenuation = 1.5f;
				this->uniformStageBuffer.cameraView[i].qudratic_attenuation = 0.19f;
				this->uniformStageBuffer.cameraView[i].intensity = 1.0f;
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	Draw panoramic texture.	*/
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  ((this->getFrameCount() % this->nrUniformBuffer) * this->nrCameraFaces) *
									  this->uniformBufferSize,
								  this->uniformBufferSize);
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glUseProgram(this->panoramic_program);

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				/*	Bind reflection cubemap.	*/
				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cameraFaceTextures[0]);

				this->scene.render();
				// this->skybox_.Render(this->camera);
			}

			//
			//	/*	Draw the scene.	*/
			//	glPolygonMode(GL_FRONT_AND_BACK, this->panoramicSettingComponent->showWireFrame ? GL_LINE : GL_FILL);
			//	/*	*/
			//	glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
			//					  ((this->getFrameCount() % this->nrUniformBuffer) * this->nrPointLights) *
			//						  this->uniformBufferSize,
			//					  this->uniformBufferSize);

			//	glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowFrameBuffers[0]);

			//	glClear(GL_DEPTH_BUFFER_BIT);
			//	glViewport(0, 0, this->panoramicWidth, this->panoramicHeight);
			//	glUseProgram(this->panoramic_program);

			//	glCullFace(GL_FRONT);
			//	glEnable(GL_CULL_FACE);
			//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			//	/*	Setup the shadow.	*/
			//	glBindVertexArray(this->refObj[0].vao);

			//	/*	*/
			//	for (size_t j = 0; j < this->refObj.size(); j++) {
			//		glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[j].nrIndicesElements, GL_UNSIGNED_INT,
			//								 (void *)(sizeof(unsigned int) * this->refObj[j].indices_offset),
			//								 this->refObj[j].vertex_offset);
			//	}
			//	glBindVertexArray(0);

			//	/*	Draw skybox.	*/
			//	{
			//		glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
			//						  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize + 0,
			//						  this->uniformBufferSize);

			//		glUseProgram(this->skybox_program);

			//		glDisable(GL_CULL_FACE);
			//		glDisable(GL_BLEND);
			//		glEnable(GL_DEPTH_TEST);
			//		glStencilMask(GL_FALSE);
			//		glDepthFunc(GL_LEQUAL);

			//		/*	*/
			//		glActiveTexture(GL_TEXTURE0);
			//		glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

			//		/*	Draw triangle.	*/
			//		glBindVertexArray(this->skybox.vao);
			//		glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			//		glBindVertexArray(0);

			//		glStencilMask(GL_TRUE);

			//		glUseProgram(0);
			//	}

			//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//
		}

		void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 1.0f, 1000.0f);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
				GL_UNIFORM_BUFFER,
				(((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->nrCameraFaces) * this->uniformBufferSize,
				this->uniformBufferSize * this->nrCameraFaces, GL_MAP_WRITE_BIT);
			glm::mat4 PointView[6];

			/*	*/
			for (size_t i = 0; i < this->nrCameraFaces; i++) {
				const glm::vec3 lightPosition = this->camera.getPosition();
				/*	*/
				glm::mat4 model = glm::mat4(1.0f);
				glm::mat4 translation = glm::translate(model, this->uniformStageBuffer.cameraView[i].position);

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
					glm::perspective(glm::radians(90.0f), (float)this->panoramicWidth / (float)this->panoramicHeight,
									 0.15f, this->uniformStageBuffer.cameraView[i].range);

				/*	*/
				this->uniformStageBuffer.model = glm::mat4(1.0f);

				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
				this->uniformStageBuffer.lightPosition = glm::vec4(this->camera.getPosition(), 0.0f);

				memcpy(&uniformPointer[i * this->uniformBufferSize], &this->uniformStageBuffer,
					   sizeof(this->uniformStageBuffer));
			}

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class PanoramicGLSample : public GLSample<Panoramic> {
	  public:
		PanoramicGLSample() : GLSample<Panoramic>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza.fbx"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PanoramicGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}