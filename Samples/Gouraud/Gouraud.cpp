#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>

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
			glm::mat4 viewProjection;
			glm::mat4 modelViewProjection;

			/*	*/
			glm::vec4 diffuseColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 ambientColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(-1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

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
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Material");
				ImGui::ColorEdit4("Tint", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Levels", &this->uniform.tessLevel, 1, 0.0f, 6.0f);
				ImGui::Checkbox("Catmull-Clark", &this->useSubdivionsCatmullClark);

				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Animate", &this->useAnimation);
			}

			bool showWireFrame = false;
			bool useSubdivionsCatmullClark = true;
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
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		/*	*/
		MeshObject sphere;

		/*	*/
		unsigned int gouraud_catmull_program;
		unsigned int gouraud_tessellation_program;

		/*	*/
		CameraController camera;

		/*	*/
		const std::string vertexGouraudShaderPath = "Shaders/gouraud/gouraud.vert.spv";
		const std::string fragmentGouraudShaderPath = "Shaders/gouraud/gouraud.frag.spv";
		const std::string controlGouraudShaderPath = "Shaders/gouraud/gouraud.tesc.spv";
		const std::string evoluationGouraudShaderPath = "Shaders/gouraud/gouraud.tese.spv";

		/*	*/
		const std::string catmullVertexGouraudShaderPath = "Shaders/gouraud/gouraud_catmull.vert.spv";
		const std::string catmullControlGouraudShaderPath = "Shaders/gouraud/gouraud_catmull.tesc.spv";
		const std::string catmullEvoluationGouraudShaderPath = "Shaders/gouraud/gouraud_catmull.tese.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->gouraud_catmull_program);
			glDeleteProgram(this->gouraud_tessellation_program);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->sphere.vao);
			glDeleteBuffers(1, &this->sphere.vbo);
			glDeleteBuffers(1, &this->sphere.ibo);
		}

		void Initialize() override {

			{
				/*	*/
				const std::vector<uint32_t> gouraud_vertex_gouraud_binary =
					IOUtil::readFileData<uint32_t>(this->vertexGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> gouraud_fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> gouraud_control_binary =
					IOUtil::readFileData<uint32_t>(this->controlGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> gouraud_evolution_binary =
					IOUtil::readFileData<uint32_t>(this->evoluationGouraudShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> catmull_control_binary =
					IOUtil::readFileData<uint32_t>(this->catmullControlGouraudShaderPath, this->getFileSystem());
				const std::vector<uint32_t> catmull_evolution_binary =
					IOUtil::readFileData<uint32_t>(this->catmullEvoluationGouraudShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->gouraud_tessellation_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &gouraud_vertex_gouraud_binary, &gouraud_fragment_binary, nullptr,
					&gouraud_control_binary, &gouraud_evolution_binary);

				/*	Load shader	*/
				this->gouraud_catmull_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &gouraud_vertex_gouraud_binary, &gouraud_fragment_binary, nullptr,
					&catmull_control_binary, &catmull_evolution_binary);
			}

			/*	Setup Shader.	*/
			glUseProgram(this->gouraud_tessellation_program);
			this->uniform_buffer_index =
				glGetUniformBlockIndex(this->gouraud_tessellation_program, "UniformBufferBlock");
			glUniformBlockBinding(this->gouraud_tessellation_program, this->uniform_buffer_index, 0);
			glUseProgram(0);

			/*	Setup Shader.	*/
			glUseProgram(this->gouraud_catmull_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->gouraud_catmull_program, "UniformBufferBlock");
			glUniformBlockBinding(this->gouraud_catmull_program, this->uniform_buffer_index, 0);
			glUseProgram(0);

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * this->nrUniformBuffer, nullptr,
						 GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			Common::loadSphere(this->sphere, 1, 8, 8);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	*/
				if (this->gouraudSettingComponent->useSubdivionsCatmullClark) {
					glUseProgram(this->gouraud_catmull_program);
				} else {
					glUseProgram(this->gouraud_tessellation_program);
				}

				/*	*/
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	*/
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);

				/*	Draw triangle   */
				glBindVertexArray(this->sphere.vao);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->gouraudSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	*/
				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, this->sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				/*	*/
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

			this->uniformStageBuffer.viewProjection = this->uniformStageBuffer.proj * this->uniformStageBuffer.view;
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;

			this->uniformStageBuffer.eyePos = glm::vec4(this->camera.getPosition(), 0);

			{
				glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
				void *uniformPointer = glMapBufferRange(
					GL_UNIFORM_BUFFER,
					((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
					this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
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