#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class PNTessellation : public GLSampleWindow {
	  public:
		PNTessellation() : GLSampleWindow() {
			this->setTitle("PNTessellation");
			this->tessellationSettingComponent =
				std::make_shared<TessellationSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->tessellationSettingComponent);
		}

		struct uniform_buffer_block {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(-1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			/*	Tessellation settings.	*/
			glm::vec4 eyePos = glm::vec4(1.0f);
			float gDisplace = 1.0f;
			float tessLevel = 1.0f;

		} uniformStageBuffer;

		class TessellationSettingComponent : public nekomimi::UIComponent {

		  public:
			TessellationSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Tessellation Settings");
			}
			void draw() override {
				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Displacement", &this->uniform.gDisplace, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Levels", &this->uniform.tessLevel, 1, 0.0f, 10.0f);
				ImGui::TextUnformatted("Light");
				ImGui::ColorEdit4("Color", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<TessellationSettingComponent> tessellationSettingComponent;

		/*	Uniform buffers.	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(uniform_buffer_block);

		/*	*/
		GeometryObject RefGeomtry;

		/*	*/
		unsigned int tessellation_pn_program;
		unsigned int wireframe_program;

		CameraController camera;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int heightmap_texture;

		/*	*/
		const std::string vertexShaderPath = "Shaders/pntessellation/pntessellation.vert.spv";
		const std::string fragmentShaderPath = "Shaders/pntessellation/pntessellation.frag.spv";
		const std::string ControlShaderPath = "Shaders/pntessellation/pntessellation.tesc.spv";
		const std::string EvoluationShaderPath = "Shaders/pntessellation/pntessellation.tese.spv";

		void Release() override {
			/*	*/
			glDeleteProgram(this->tessellation_pn_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->heightmap_texture);

			/*	*/
			glDeleteVertexArrays(1, &this->RefGeomtry.vao);
			glDeleteBuffers(1, &this->RefGeomtry.vbo);
			glDeleteBuffers(1, &this->RefGeomtry.ibo);

			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string diffuseTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string heightTexturePath = this->getResult()["heightmap"].as<std::string>();
			const std::string modelPath = this->getResult()["model"].as<std::string>();

			/*	*/
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
			const std::vector<uint32_t> control_source =
				IOUtil::readFileData<uint32_t>(this->ControlShaderPath, this->getFileSystem());
			const std::vector<uint32_t> evolution_source =
				IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->tessellation_pn_program = ShaderLoader::loadGraphicProgram(
				compilerOptions, &vertex_source, &fragment_source, nullptr, &control_source, &evolution_source);

			/*	Setup Shader.	*/
			glUseProgram(this->tessellation_pn_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->tessellation_pn_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->tessellation_pn_program, "diffuse"), 0);
			glUniform1i(glGetUniformLocation(this->tessellation_pn_program, "gDisplacementMap"), 1);
			glUniformBlockBinding(this->tessellation_pn_program, this->uniform_buffer_index, 0);

			glUseProgram(0);

			/*	Load Diffuse and Height Map Texture.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);
			this->heightmap_texture = textureImporter.loadImage2D(heightTexturePath);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align(this->uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->RefGeomtry.vao);
			glBindVertexArray(this->RefGeomtry.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->RefGeomtry.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, RefGeomtry.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glGenBuffers(1, &this->RefGeomtry.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RefGeomtry.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->RefGeomtry.nrIndicesElements = indices.size();

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

		void draw() override {
			int width, height;
			getSize(&width, &height);

			this->uniformStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize,
								  this->uniformSize);

				glUseProgram(this->tessellation_pn_program);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->heightmap_texture);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				/*	Draw triangle*/
				glBindVertexArray(this->RefGeomtry.vao);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->tessellationSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, this->RefGeomtry.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				glBindVertexArray(0);
				glUseProgram(0);
			}
		}

		void update() override {
			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			this->uniformStageBuffer.model = glm::mat4(1.0f);
			this->uniformStageBuffer.model = glm::translate(this->uniformStageBuffer.model, glm::vec3(0, 0, 10));
			this->uniformStageBuffer.model =
				glm::rotate(this->uniformStageBuffer.model, (float)Math::PI_half, glm::vec3(1, 0, 0));
			this->uniformStageBuffer.model = glm::scale(this->uniformStageBuffer.model, glm::vec3(10, 10, 10));

			this->uniformStageBuffer.view = this->camera.getViewMatrix();
			this->uniformStageBuffer.modelViewProjection =
				this->uniformStageBuffer.proj * this->uniformStageBuffer.view * this->uniformStageBuffer.model;
			this->uniformStageBuffer.eyePos = glm::vec4(this->camera.getPosition(), 0);

			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniformStageBuffer, sizeof(this->uniformStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
	class PNTessellationGLSample : public GLSample<PNTessellation> {
	  public:
		PNTessellationGLSample() : GLSample<PNTessellation>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path",
					cxxopts::value<std::string>()->default_value("asset/tessellation_diffusemap.png"))(
				"H,heightmap", "Height Map Texture Path",
				cxxopts::value<std::string>()->default_value("asset/tessellation_heightmap.png"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PNTessellationGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}