#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class ConditionalScene : public Scene {
	  public:
	};

	/**
	 * @brief
	 *
	 */
	class ConditionalDraw : public GLSampleWindow {
	  public:
		ConditionalDraw() : GLSampleWindow() {
			this->setTitle("Conditional Draw");
			this->conditionalSettingComponent =
				std::make_shared<ConditionalDrawSettingComponent>(this->uniformStageBuffer);

			this->addUIComponent(this->conditionalSettingComponent);
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			/*	*/
			glm::vec4 tintColor = glm::vec4(1, 1, 1, 1);

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	Normal attirbutes.	*/
			float normalStrength = 1.0f;

		} uniformStageBuffer;

		/*	*/
		MeshObject plan;
		ConditionalScene scene;

		/*	Textures.	*/
		unsigned int diffuse_texture;
		unsigned int normal_texture;

		/*	*/
		unsigned int normalMapping_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(uniform_buffer_block);

		CameraController camera;

		class ConditionalDrawSettingComponent : public nekomimi::UIComponent {
		  public:
			ConditionalDrawSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Conditional Draw");
			}

			void draw() override {
				ImGui::ColorEdit4("Tint Color", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Normal Setting");
				ImGui::DragFloat("Strength", &this->uniform.normalStrength);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			bool showBoundingBoxes = false;
			bool showOverlapping = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<ConditionalDrawSettingComponent> conditionalSettingComponent;

		const std::string vertexShaderPath = "Shaders/normalmap/normalmap.vert.spv";
		const std::string fragmentShaderPath = "Shaders/normalmap/normalmap.frag.spv";

		void Release() override {
			this->scene.release();

			/*	*/
			glDeleteProgram(this->normalMapping_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			/*	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		void Initialize() override {

			/*	*/
			const std::string modelPath = this->getResult()["model"].as<std::string>();
			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string normalTexturePath = this->getResult()["normal-texture"].as<std::string>();

			/*	*/
			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->normalMapping_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->normalMapping_program);
			unsigned int uniform_buffer_index =
				glGetUniformBlockIndex(this->normalMapping_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->normalMapping_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->normalMapping_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->normalMapping_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);
			this->normal_texture = textureImporter.loadImage2D(normalTexturePath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = fragcore::Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->normalMapping_program);

				glDisable(GL_CULL_FACE);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, conditionalSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			{
				/*	*/
				this->uniformStageBuffer.model = glm::mat4(1.0f);
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, glm::radians(elapsedTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(10.95f));
				this->uniformStageBuffer.view = this->camera.getViewMatrix();
				this->uniformStageBuffer.modelViewProjection =
					this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
				this->uniformStageBuffer.ViewProj = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
			}

			{
				/*	*/
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
					this->uniformBufferSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class ConditionalDrawGLSample : public GLSample<ConditionalDraw> {
	  public:
		ConditionalDrawGLSample() : GLSample<ConditionalDraw>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/diffuse.png"))(
				"N,normal-texture", "NormalMap Path",
				cxxopts::value<std::string>()->default_value("asset/normalmap.png"))(
				"M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::ConditionalDrawGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}