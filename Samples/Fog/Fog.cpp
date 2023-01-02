
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Fog : public GLSampleWindow {
	  public:
		Fog() : GLSampleWindow() {
			this->setTitle("Fog");
			this->fogSettingComponent = std::make_shared<FogSettingComponent>(this->uniform);
			this->addUIComponent(this->fogSettingComponent);
		}

		enum class FogType : unsigned int { None, Linear, Exp, Exp2, Height };

		struct UniformBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			glm::vec4 fogColor = glm::vec4(1, 0, 0, 1);
			float cameraNear = 0.15f;
			float cameraFar = 1000.0f;
			float fogStart = 100;
			float fogEnd = 1000;
			float fogDensity = 0.1f;
			FogType fogType = FogType::Exp;
			float fogIntensity = 1.0f;
		} uniform;

		std::string diffuseTexturePath = "asset/diffuse.png";

		unsigned int diffuse_texture;

		std::vector<GeometryObject> refObj;

		unsigned int graphic_fog_program;

		/*	Uniform Buffer.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class FogSettingComponent : public nekomimi::UIComponent {
		  public:
			FogSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Fog Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("Light Settings");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Fog Settings");
				ImGui::DragInt("Fog Type", (int *)&this->uniform.fogType);
				ImGui::ColorEdit4("Fog Color", &this->uniform.fogColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Fog Density", &this->uniform.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->uniform.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->uniform.fogStart);
				ImGui::DragFloat("Fog End", &this->uniform.fogEnd);
				ImGui::TextUnformatted("Debug Settings");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<FogSettingComponent> fogSettingComponent;

		const std::string modelPath = "asset/sponza/sponza.obj";
		/*	*/
		const std::string vertexGraphicShaderPath = "Shaders/fog/fog.vert";
		const std::string fragmentGraphicShaderPath = "Shaders/fog/fog.frag";

		virtual void Release() override {
			glDeleteProgram(this->graphic_fog_program);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	*/
			std::vector<char> vertex_source =
				IOUtil::readFileString(this->vertexGraphicShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				IOUtil::readFileString(this->fragmentGraphicShaderPath, this->getFileSystem());

			/*	Load shaders	*/
			this->graphic_fog_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(this->diffuseTexturePath);

			/*	*/
			glUseProgram(this->graphic_fog_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->graphic_fog_program, "UniformBufferBlock");
			glUniform1iARB(glGetUniformLocation(this->graphic_fog_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->graphic_fog_program, this->uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			// Create uniform buffer.
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	*/
			ModelImporter modelLoader(FileSystem::getFileSystem());
			modelLoader.loadContent(modelPath, 0);

			ImportHelper::loadModelBuffer(modelLoader, refObj);
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->uniform.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height,
												  this->uniform.cameraNear, this->uniform.cameraFar);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			{
				/*	*/
				glViewport(0, 0, width, height);

				/*	*/
				glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glUseProgram(this->graphic_fog_program);

				glCullFace(GL_BACK);
				glDisable(GL_CULL_FACE);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, fogSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				glBindVertexArray(this->refObj[0].vao);
				for (size_t i = 0; i < this->refObj.size(); i++) {
					glDrawElementsBaseVertex(GL_TRIANGLES, this->refObj[i].nrIndicesElements, GL_UNSIGNED_INT,
											 (void *)(sizeof(unsigned int) * this->refObj[i].indices_offset),
											 this->refObj[i].vertex_offset);
				}
				glBindVertexArray(0);
			}
		}

		virtual void update() {
			/*	Update Camera.	*/
			this->camera.update(getTimer().deltaTime());

			/*	*/
			this->uniform.model = glm::mat4(1.0f);
			// this->mvp.model = glm::scale(this->mvp.model, glm::vec3(1.95f));
			this->uniform.view = this->camera.getViewMatrix();
			this->uniform.modelViewProjection = this->uniform.proj * this->uniform.view * this->uniform.model;

			/*	*/
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform, sizeof(uniform));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};

	class FogGLSample : public GLSample<Fog> {
	  public:
		FogGLSample(int argc, const char **argv) : GLSample<Fog>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("Texture-Sample")("T,texture", "Texture Path",
												  cxxopts::value<std::string>()->default_value("texture.png"))(
				"N,normal map", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::FogGLSample sample(argc, argv);
		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}