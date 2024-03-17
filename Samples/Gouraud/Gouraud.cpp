#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Gouraud : public GLSampleWindow {
	  public:
		Gouraud() : GLSampleWindow() {
			this->setTitle("Gouraud");

			/*	*/
			this->gouraudSettingComponent = std::make_shared<GouraudSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->gouraudSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			glm::vec4 diffuseColor = glm::vec4(1, 1, 1, 1);

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(-1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			/*	Tessellation settings.	*/
			glm::vec4 eyePos = glm::vec4(1.0f);
			float tessLevel = 1.0f;

		} uniformStageBuffer;

		class GouraudSettingComponent : public nekomimi::UIComponent {

		  public:
			GouraudSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Tessellation Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Light");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Levels", &this->uniform.tessLevel, 1, 0.0f, 6.0f);
				ImGui::Checkbox("Catmull-Clark", &this->subdivionsCatmullClark);

				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Animate", &this->useAnimation);
			}

			bool showWireFrame = false;
			bool subdivionsCatmullClark = true;
			bool useAnimation = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<GouraudSettingComponent> gouraudSettingComponent;

		/*	Uniform buffers.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(uniform_buffer_block);

		/*	*/
		MeshObject sphere;

		/*	*/
		unsigned int gouraud_program;

		/*	*/
		CameraController camera;

		/*	*/
		const std::string vertexGouraudShaderPath = "Shaders/gouraud/gouraud.vert.spv";
		const std::string fragmentGouraudShaderPath = "Shaders/gouraud/gouraud.frag.spv";
		const std::string ControlGouraudShaderPath = "Shaders/gouraud/gouraud.tesc.spv";
		const std::string EvoluationGouraudShaderPath = "Shaders/gouraud/gouraud.tese.spv";
		// const std::string ControlShaderPath = "Shaders/gouraud/gouraud.tesc.spv";
		// const std::string EvoluationShaderPath = "Shaders/gouraud/gouraud.tese.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->gouraud_program);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->sphere.vao);
			glDeleteBuffers(1, &this->sphere.vbo);
			glDeleteBuffers(1, &this->sphere.ibo);
		}

		void Initialize() override {

			{
				/*	*/
				const std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> control_binary =
					IOUtil::readFileData<uint32_t>(this->ControlGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> evolution_binary =
					IOUtil::readFileData<uint32_t>(this->EvoluationGouraudShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->gouraud_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_binary, &fragment_binary, nullptr, &control_binary, &evolution_binary);
			}

			/*	Setup Shader.	*/
			glUseProgram(this->gouraud_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->gouraud_program, "UniformBufferBlock");
			glUniformBlockBinding(this->gouraud_program, this->uniform_buffer_index, 0);
			glUseProgram(0);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align(this->uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			{
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generateSphere(1, vertices, indices, 8, 8);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->sphere.vao);
				glBindVertexArray(this->sphere.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->sphere.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, sphere.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->sphere.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->sphere.nrIndicesElements = indices.size();

				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				/*	UV.	*/
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(12));

				/*	Normal.	*/
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(20));

				/*	Tangent.	*/
				glEnableVertexAttribArray(3);
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									  reinterpret_cast<void *>(32));

				glBindVertexArray(0);
			}
		}

		void onResize(int width, int height) override {
			/*	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize,
								  this->uniformSize);

				glUseProgram(this->gouraud_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				/*	Draw triangle   */
				glBindVertexArray(this->sphere.vao);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->gouraudSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, this->sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				glBindVertexArray(0);
				glUseProgram(0);
			}
		}

		void update() override {

			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update uniform stage buffer values.	*/
			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::translate(this->uniformStageBuffer.model, glm::vec3(0, 0, 10));
			if (this->gouraudSettingComponent->useAnimation) {
				this->uniformStageBuffer.model = glm::rotate(
					this->uniformStageBuffer.model, (float)Math::PI_half * elapsedTime * 0.12f, glm::vec3(0, 1, 0));
			}
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(10, 10, 10));
			this->uniformStageBuffer.proj = this->camera.getProjectionMatrix();
			this->uniformStageBuffer.view = this->camera.getViewMatrix();

			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			this->uniformStageBuffer.eyePos = glm::vec4(this->camera.getPosition(), 0);

			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
					this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
				memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}
	};

	class GouraudGLSample : public GLSample<Gouraud> {
	  public:
		GouraudGLSample() : GLSample<Gouraud>() {}
		void customOptions(cxxopts::OptionAdder &options) override {}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::GouraudGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}