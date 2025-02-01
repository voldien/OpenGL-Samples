#include "GLUIComponent.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Irradiance : public GLSampleWindow {

	  public:
		Irradiance() : GLSampleWindow() {
			this->setTitle("Irradiance");

			this->skyboxSettingComponent = std::make_shared<IrradianceSettingComponent>(*this);
			this->addUIComponent(this->skyboxSettingComponent);

			this->camera.setPosition(glm::vec3(25.0f));
			this->camera.lookAt(glm::vec3(0.f));

			this->camera.enableNavigation(true);
		}

		struct uniform_buffer_block {
			glm::mat4 proj{};
			glm::mat4 modelViewProjection{};
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
			float gamma = 2.2;
		} uniform_stage_buffer;

		Skybox skybox;
		MeshObject sphere;

		unsigned int display_graphic_program{};

		unsigned int irradiance_texture = 0;
		unsigned int skybox_texture_panoramic = 0;

		CameraController camera;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(uniform_buffer_block);

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/irradiance/irradiance.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

	  public:
		class IrradianceSettingComponent : public GLUIComponent<Irradiance> {

		  public:
			IrradianceSettingComponent(Irradiance &sample)
				: GLUIComponent<Irradiance>(sample, "Irradiance Settings"), uniform(sample.uniform_stage_buffer) {}
			void draw() override {
				ImGui::ColorEdit4("Tint", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Image(static_cast<ImTextureID>(this->getRefSample().irradiance_texture), ImVec2(512, 256),
							 ImVec2(1, 1), ImVec2(0, 0));
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<IrradianceSettingComponent> skyboxSettingComponent;

		void Release() override {
			glDeleteProgram(this->display_graphic_program);
			glDeleteTextures(1, (const GLuint *)&this->skybox_texture_panoramic);
			glDeleteTextures(1, (const GLuint *)&this->irradiance_texture);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["texture"].as<std::string>();

			{
				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create skybox graphic pipeline program.	*/
				this->display_graphic_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->display_graphic_program);
			unsigned int uniform_buffer_index =
				glGetUniformBlockIndex(this->display_graphic_program, "UniformBufferBlock");
			glUniformBlockBinding(this->display_graphic_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->display_graphic_program, "PanoramaTexture"), 0);
			glUseProgram(0);

			/*	Load panoramic texture.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_texture_panoramic = textureImporter.loadImage2D(panoramicPath, ColorSpace::SRGB);

			ProcessData util(this->getFileSystem());
			util.computeIrradiance(this->skybox_texture_panoramic, this->irradiance_texture, 256, 128);

			skybox.Init(this->skybox_texture_panoramic, Skybox::loadDefaultProgram(this->getFileSystem()));

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align<size_t>(this->uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			Common::loadSphere(this->sphere, 3, 16, 16);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			/*	*/
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, skyboxSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			{

				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize,
								  this->uniformSize);

				/*	*/
				glUseProgram(this->display_graphic_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->sphere.vao);
				glDrawElements(GL_TRIANGLES, this->sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {
			/*	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniform_stage_buffer.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.modelViewProjection =
				(this->uniform_stage_buffer.proj * this->camera.getViewMatrix());

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class IrradianceGLSample : public GLSample<Irradiance> {
	  public:
		IrradianceGLSample() : GLSample<Irradiance>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::IrradianceGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
