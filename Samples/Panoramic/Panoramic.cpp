#include "Math/Math.h"
#include "Scene.h"
#include "Skybox.h"
#include "Util/ProcessDataUtil.h"
#include "imgui.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ModelViewer.h>
#include <ShaderLoader.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace glsample {

	/**
	 * @brief Render Scene as Panoramic
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
			this->camera.setPosition(glm::vec3(0.0f));

			this->camera.enableLook(true);
			this->camera.enableNavigation(true);
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProjection[6];
			glm::mat4 modelViewProjection;

			/*	Light source.	*/
			glm::vec4 direction = glm::vec4(0.7, 0.7, 1, 1);
			glm::vec4 lightColor = glm::vec4(1, 1, 1, 1);

			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 ambientColor = glm::vec4(0.3, 0.3, 0.3, 1);
			glm::vec4 viewDir;

			float shininess = 8.0f;
		} uniformStageBuffer;

		/*	Point light shadow maps.	*/
		unsigned int panoramicFrameBuffer;
		unsigned int panoramicCubeMapTexture;
		unsigned int depthTexture;

		/*	*/
		unsigned int panoramicWidth = 1024;
		unsigned int panoramicHeight = 1024;

		unsigned int irradiance_texture;

		MeshObject plan;
		Scene scene;
		Skybox skybox;

		/*	*/
		unsigned int graphic_cubemap_program; /*	*/
		unsigned int panoramic_program;		  /*	Show panoramic.	*/
		unsigned int skybox_program;		  /*	*/

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class PanoramicSettingComponent : public nekomimi::UIComponent {
		  public:
			PanoramicSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Panoramic Settings");
			}
			void draw() override {

				ImGui::TextUnformatted("Lightning");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Material");
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0], ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::TextUnformatted("Depth Texture");
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
		const std::string geomtryPanoramicShaderPath = "Shaders/panoramic/panoramic.geom.spv";
		const std::string fragmentPanoramicShaderPath = "Shaders/panoramic/panoramic.frag.spv";

		/*	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	*/
		const std::string vertexOverlayShaderPath = "Shaders/postprocessingeffects/overlay.vert.spv";
		const std::string fragmentCubemap2PanoramicShaderPath = "Shaders/panoramic/cubempa2panoramic.frag.spv";

		void Release() override {
			glDeleteProgram(this->graphic_cubemap_program);
			glDeleteProgram(this->panoramic_program);

			glDeleteFramebuffers(1, &this->panoramicFrameBuffer);
			glDeleteTextures(1, &this->panoramicCubeMapTexture);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {
			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	*/
				const std::vector<uint32_t> vertex_panoramic_binary =
					IOUtil::readFileData<uint32_t>(this->vertexPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geometry_panoramic_binary =
					IOUtil::readFileData<uint32_t>(this->geomtryPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_panoramic_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentPanoramicShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_overlay_binary =
					IOUtil::readFileData<uint32_t>(this->vertexOverlayShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_cubemap_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentCubemap2PanoramicShaderPath, this->getFileSystem());

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->panoramic_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_panoramic_binary, &fragment_panoramic_binary, &geometry_panoramic_binary);

				this->graphic_cubemap_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_overlay_binary, &fragment_skybox_cubemap_binary);

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

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->graphic_cubemap_program);
			glUniform1i(glGetUniformLocation(this->graphic_cubemap_program, "textureCubeMap"), 0);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	 Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create panoramic layered cubemap.	*/
			{

				glGenFramebuffers(1, &this->panoramicFrameBuffer);

				glGenTextures(1, &this->panoramicCubeMapTexture);

				glBindFramebuffer(GL_FRAMEBUFFER, this->panoramicFrameBuffer);

				/*	*/
				glBindTexture(GL_TEXTURE_CUBE_MAP, this->panoramicCubeMapTexture);

				/*	Setup each face.	*/
				for (size_t i = 0; i < 6; i++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, this->panoramicWidth,
								 this->panoramicHeight, 0, GL_RGB, GL_FLOAT, nullptr);
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
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->panoramicCubeMapTexture, 0);

				glDrawBuffer(GL_COLOR_ATTACHMENT0);

				/*	Depth Texture.	*/
				glGenTextures(1, &this->depthTexture);
				glBindTexture(GL_TEXTURE_CUBE_MAP, this->depthTexture);
				/*	Setup each face.	*/
				for (size_t i = 0; i < 6; i++) {
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, this->panoramicWidth,
								 this->panoramicHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
				}
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->depthTexture, 0);

				/*	*/
				const int frstat = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (frstat != GL_FRAMEBUFFER_COMPLETE) {
					/*  Delete  */
					throw RuntimeException("Failed to create framebuffer, {}",
										   (const char *)glewGetErrorString(frstat));
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			/*	load Skybox Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath);
			this->skybox.Init(skytexture, this->skybox_program);

			ProcessData util(this->getFileSystem());
			util.computeIrradiance(skytexture, this->irradiance_texture, 256, 128);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*	Load geometry.	*/
			Common::loadPlan(this->plan, 1, 1, 1);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			glBindFramebuffer(GL_FRAMEBUFFER, this->panoramicFrameBuffer);

			/*	Draw panoramic texture.	*/
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  ((this->getFrameCount() % this->nrUniformBuffer)) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);
				/*	*/
				glViewport(0, 0, panoramicWidth, panoramicHeight);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

				glUseProgram(this->panoramic_program);

				glActiveTexture(GL_TEXTURE0 + 10);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				this->scene.render();
			}

			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glViewport(0, 0, width, height);

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_CUBE_MAP, this->panoramicCubeMapTexture);

				/*	Draw panoramic.	*/
				glUseProgram(this->graphic_cubemap_program);
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glUseProgram(0);
			}

			// this->skybox.Render(this->camera);
		}

		void update() override {
			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			uint8_t *uniformPointer = (uint8_t *)glMapBufferRange(
				GL_UNIFORM_BUFFER,
				(((this->getFrameCount() + 1) % this->nrUniformBuffer)) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT);

			/*	Compute light matrices.	*/
			this->camera.setFOV(90);
			glm::mat4 pointPer = glm::perspective(
				glm::radians(90.0f), (float)this->panoramicWidth / (float)this->panoramicHeight, 0.45f, 2000.0f);

			/*	*/
			const glm::vec3 cameraPosition = this->camera.getPosition();

			/*	*/
			this->uniformStageBuffer.ViewProjection[0] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
			this->uniformStageBuffer.ViewProjection[1] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
			/*	*/
			this->uniformStageBuffer.ViewProjection[2] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
			this->uniformStageBuffer.ViewProjection[3] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
			/*	*/
			this->uniformStageBuffer.ViewProjection[4] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
			this->uniformStageBuffer.ViewProjection[5] =
				glm::lookAt(cameraPosition, cameraPosition + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

			for (int i = 0; i < 6; i++) {
				this->uniformStageBuffer.ViewProjection[i] = glm::rotate(
					this->uniformStageBuffer.ViewProjection[i], glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				this->uniformStageBuffer.ViewProjection[i] =
					pointPer * glm::scale(this->uniformStageBuffer.ViewProjection[i], glm::vec3(1, 1, 1));
			}

			/*	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);

			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			this->uniformStageBuffer.viewDir = glm::vec4(this->camera.getLookDirection(), 0);

			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class PanoramicGLSample : public GLSample<Panoramic> {
	  public:
		PanoramicGLSample() : GLSample<Panoramic>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"S,skybox", "Skybox Texture File Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
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