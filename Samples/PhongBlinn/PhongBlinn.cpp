#include "ModelImporter.h"
#include "Scene.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class PhongBlinn : public GLSampleWindow {
	  public:
		PhongBlinn() : GLSampleWindow() {
			this->setTitle("Phong & Blinn");

			this->phongblinnSettingComponent = std::make_shared<PhongBlinnSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->phongblinnSettingComponent);

			/*	*/
			this->camera.setPosition(glm::vec3(15.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		using PointLight = struct point_light_t {
			glm::vec3 position;
			float range;
			glm::vec4 color;
			float intensity;
			float constant_attenuation;
			float linear_attenuation;
			float qudratic_attenuation;
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 modelViewProjection{};

			/*	*/
			glm::vec4 ambientColor = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 viewPos = glm::vec4(0);

			/*	light source.	*/
			PointLight pointLights[4]{};

			float shininess = 8;
		} uniformStageBuffer;

		/*	*/
		Scene scene; /*	World Scene.	*/
		const size_t nrPointLights = 4;

		/*	Textures.	*/
		unsigned int diffuse_texture{};

		/*	*/
		unsigned int phong_program{};
		unsigned int blinn_program{};

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class PhongBlinnSettingComponent : public nekomimi::UIComponent {

		  public:
			PhongBlinnSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Phong-Blinn Settings");
			}
			void draw() override {

				for (size_t i = 0; i < sizeof(uniform.pointLights) / sizeof(uniform.pointLights[0]); i++) {
					ImGui::PushID(1000 + i);
					ImGui::TextUnformatted("Point Light Setting");
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightVisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Color", &this->uniform.pointLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Position", &this->uniform.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation", &this->uniform.pointLights[i].constant_attenuation);
						ImGui::DragFloat(" Range", &this->uniform.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.pointLights[i].intensity);
					}
					ImGui::PopID();
				}
				/*	*/
				ImGui::TextUnformatted("Ambient Light Setting");
				ImGui::ColorEdit4("Ambient Color", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->uniform.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				ImGui::TextUnformatted("Material Setting");
				ImGui::DragFloat("Shininess", &this->uniform.shininess);
				ImGui::Checkbox("Use Blinn (!Phong)", &this->useBlinn);

				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("Rotate", &this->useAnimate);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}
			bool useBlinn = false;
			bool useAnimate = false;
			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
			bool lightVisible[4] = {true, true, true, true};
		};
		std::shared_ptr<PhongBlinnSettingComponent> phongblinnSettingComponent;

		const std::string vertexShaderPath = "Shaders/phongblinn/phongblinn.vert.spv";
		const std::string fragmentPhongShaderPath = "Shaders/phongblinn/phong.frag.spv";
		const std::string fragmentBlinnShaderPath = "Shaders/phongblinn/blinn.frag.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->phong_program);
			glDeleteProgram(this->blinn_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			/*	*/
			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_phongblinn_binary_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_phong_binary_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentPhongShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_blinn_binary_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentBlinnShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->phong_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_phongblinn_binary_binary, &fragment_phong_binary_binary);

				this->blinn_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_phongblinn_binary_binary, &fragment_blinn_binary_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->phong_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->phong_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->phong_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->phong_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->blinn_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->blinn_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->blinn_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->blinn_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath, ColorSpace::SRGB);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize =
				fragcore::Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load scene from model importer.	*/
			ModelImporter modelLoader = ModelImporter(this->getFileSystem());
			modelLoader.loadContent(modelPath, 0);
			this->scene = Scene::loadFrom(modelLoader);

			/*  Init lights.    */
			const glm::vec4 colors[] = {glm::vec4(1, 0.1, 0.1, 1), glm::vec4(0.1, 1, 0.1, 1), glm::vec4(0.1, 0.1, 1, 1),
										glm::vec4(1, 0.1, 1, 1)};
			for (size_t i = 0; i < this->nrPointLights; i++) {
				/*	*/
				uniformStageBuffer.pointLights[i].range = 25.0f;
				uniformStageBuffer.pointLights[i].position =
					glm::vec3(i * -1.0f, i * 1.0f, i * -1.5f) * 12.0f + glm::vec3(1.0f, 1.0f, 1.0f);
				uniformStageBuffer.pointLights[i].color = colors[i];
				uniformStageBuffer.pointLights[i].constant_attenuation = 1.0f;
				uniformStageBuffer.pointLights[i].linear_attenuation = 0.1f;
				uniformStageBuffer.pointLights[i].qudratic_attenuation = 0.05f;
				uniformStageBuffer.pointLights[i].intensity = 2.0f;
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	*/
				if (this->phongblinnSettingComponent->useBlinn) {
					glUseProgram(this->blinn_program);
				} else {
					glUseProgram(this->phong_program);
				}

				glDisable(GL_CULL_FACE);

				/*	Bind texture.   */
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->phongblinnSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				this->scene.render();
			}
		}

		void update() override {
			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniform stage buffer values.	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			if (this->phongblinnSettingComponent->useAnimate) {
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(12.0f * elapsedTime), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(1.95f));
			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();

			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			this->uniformStageBuffer.viewPos = glm::vec4(this->camera.getPosition(), 0.0);

			/*	Update uniform buffer.	*/
			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class PhongBlinnGLSample : public GLSample<PhongBlinn> {
	  public:
		PhongBlinnGLSample() : GLSample<PhongBlinn>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/bunny/bunny.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PhongBlinnGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}