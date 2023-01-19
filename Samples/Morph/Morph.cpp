#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class Morph : public GLSampleWindow {
	  public:
		Morph() : GLSampleWindow() {
			this->setTitle("Morph");
			tessellationSettingComponent = std::make_shared<TessellationSettingComponent>(this->mvp);
			this->addUIComponent(tessellationSettingComponent);
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(-1.0f / std::sqrt(2.0f), -1.0f / std::sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			/*	Tessellation settings.	*/
			glm::vec3 eyePos = glm::vec3(1.0f);
			float gDisplace = 1.0f;
			float tessLevel = 1.0f;

		} mvp;

		class TessellationSettingComponent : public nekomimi::UIComponent {

		  public:
			TessellationSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Tessellation Settings");
			}
			virtual void draw() override {
				int nrMorphTargets = 3;
				for (size_t i = 0; i < nrMorphTargets; i++) {
					ImGui::DragFloat("Morph Target", &this->uniform.gDisplace, 1, 0.0f, 100.0f);
				}
				ImGui::DragFloat("Displacement", &this->uniform.gDisplace, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Tessellation Levels", &this->uniform.tessLevel, 1, 0.0f, 10.0f);
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);

				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<TessellationSettingComponent> tessellationSettingComponent;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		/*	*/
		unsigned int vbo;
		unsigned int ibo;
		unsigned int vao;
		unsigned int nrElements;
		/*	*/
		unsigned int tessellation_program;
		unsigned int wireframe_program;

		CameraController camera;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int heightmap_texture;

		const std::string diffuseTexturePath = "asset/tessellation_diffusemap.png";
		const std::string heightTexturePath = "asset/tessellation_heightmap.png";
		/*	*/
		const std::string vertexShaderPath = "Shaders/tessellation/tessellation.vert";
		const std::string fragmentShaderPath = "Shaders/tessellation/tessellation.frag";
		const std::string ControlShaderPath = "Shaders/tessellation/tessellation.tesc";
		const std::string EvoluationShaderPath = "Shaders/tessellation/tessellation.tese";

		virtual void Release() override {
			glDeleteProgram(this->tessellation_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->heightmap_texture);

			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
			glDeleteBuffers(1, &this->ibo);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		virtual void Initialize() override {

			/*	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);
			std::vector<char> control_source = IOUtil::readFile(ControlShaderPath);
			std::vector<char> evolution_source = IOUtil::readFile(EvoluationShaderPath);

			/*	Load shader	*/
			this->tessellation_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source, nullptr,
																		  &control_source, &evolution_source);

			/*	Setup Shader.	*/
			glUseProgram(this->tessellation_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->tessellation_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->tessellation_program, "diffuse"), 0);
			glUniform1i(glGetUniformLocation(this->tessellation_program, "gDisplacementMap"), 1);
			glUniformBlockBinding(this->tessellation_program, this->uniform_buffer_index, 0);

			glUseProgram(0);

			/*	Load Diffuse and Height Map Texture.	*/
			this->diffuse_texture = TextureImporter::loadImage2D(this->diffuseTexturePath);
			this->heightmap_texture = TextureImporter::loadImage2D(this->heightTexturePath);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize += minMapBufferSize - (uniformSize % minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			glGenBuffers(1, &this->ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
						 GL_STATIC_DRAW);
			this->nrElements = indices.size();

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

			glBindVertexArray(0);
		}

		virtual void draw() override {
			this->mvp.proj = glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.15f, 1000.0f);

			

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			glUseProgram(this->tessellation_program);

			/*	*/
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

			/*	*/
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, this->heightmap_texture);

			glDisable(GL_CULL_FACE);
			glEnable(GL_DEPTH_TEST);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
			glDrawElements(GL_PATCHES, nrElements, GL_UNSIGNED_INT, nullptr);

			/*	Draw wireframe outline.	*/
			if (this->tessellationSettingComponent->showWireFrame) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawElements(GL_PATCHES, nrElements, GL_UNSIGNED_INT, nullptr);
			}
			glBindVertexArray(0);
		}

		virtual void update() {
			/*	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			this->mvp.model = glm::mat4(1.0f);
			this->mvp.model = glm::translate(this->mvp.model, glm::vec3(0, 0, 10));
			this->mvp.model = glm::rotate(this->mvp.model, (float)Math::PI_half, glm::vec3(1, 0, 0));
			this->mvp.model = glm::scale(this->mvp.model, glm::vec3(10, 10, 10));

			this->mvp.view = camera.getViewMatrix();
			this->mvp.modelViewProjection = this->mvp.proj * this->mvp.view * this->mvp.model;
			this->mvp.eyePos = camera.getPosition();

			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->mvp, sizeof(mvp));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

} // namespace glsample

// TODO add custom image support.

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::Morph> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}