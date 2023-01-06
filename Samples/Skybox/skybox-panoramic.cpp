#include "GLSampleWindow.h"

#include "ImageImport.h"
#include "ShaderLoader.h"
#include <GL/glew.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class SkyBoxPanoramic : public GLSampleWindow {

	  public:
		SkyBoxPanoramic() : GLSampleWindow() {
			this->setTitle("SkyBoxPanoramic");

			this->skyboxSettingComponent = std::make_shared<SkyboxPanoramicSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->skyboxSettingComponent);

			this->camera.enableNavigation(false);
		}

		// TODO use.
		GeometryObject SkyboxCube;

		unsigned int skybox_program;

		struct UniformBufferBlock {
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor;
			float exposure = 1.0f;
		} uniform_stage_buffer;

		glm::mat4 proj;
		int skybox_panoramic;
		std::string panoramicPath = "asset/panoramic.jpg";
		CameraController camera;

		// TODO change to vector
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

	  public:
		class SkyboxPanoramicSettingComponent : public nekomimi::UIComponent {

		  public:
			SkyboxPanoramicSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("SkyBox Settings");
			}
			virtual void draw() override {
				ImGui::ColorEdit4("Tint", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Exposure", &this->uniform.exposure);
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SkyboxPanoramicSettingComponent> skyboxSettingComponent;

		virtual void Release() override {
			glDeleteProgram(this->skybox_program);
			glDeleteVertexArrays(1, &this->SkyboxCube.vao);
			glDeleteBuffers(1, &this->SkyboxCube.vbo);
			glDeleteBuffers(1, &this->SkyboxCube.ibo);
			glDeleteTextures(1, (const GLuint *)&this->skybox_panoramic);
		}

		virtual void Initialize() override {
			/*	Load shader	*/

			std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			std::vector<char> vertex_source_T = fragcore::ShaderCompiler::convertSPIRV(vertex_source, compilerOptions);
			std::vector<char> fragment_source_T =
				fragcore::ShaderCompiler::convertSPIRV(fragment_source, compilerOptions);

			/*	*/
			this->skybox_program = ShaderLoader::loadGraphicProgram(&vertex_source_T, &fragment_source_T);

			/*	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUniform1iARB(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_panoramic = textureImporter.loadImage2D(this->panoramicPath);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize = Math::align(uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferARB(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1.0f, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->SkyboxCube.vao);
			glBindVertexArray(this->SkyboxCube.vao);

			/*	*/
			glGenBuffers(1, &this->SkyboxCube.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SkyboxCube.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->SkyboxCube.nrIndicesElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->SkyboxCube.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, SkyboxCube.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);
			/*	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
									 reinterpret_cast<void *>(12));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			this->update();

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, skyboxSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	*/
			glUseProgram(this->skybox_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_panoramic);

			/*	Draw triangle.	*/
			glBindVertexArray(this->SkyboxCube.vao);
			glDrawElements(GL_TRIANGLES, this->SkyboxCube.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		virtual void update() {
			/*	*/
			camera.update(getTimer().deltaTime());

			this->proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniform_stage_buffer.modelViewProjection = (this->proj * camera.getViewMatrix());

			glBindBufferARB(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(uniform_stage_buffer));
			glUnmapBufferARB(GL_UNIFORM_BUFFER);
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::SkyBoxPanoramic> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
