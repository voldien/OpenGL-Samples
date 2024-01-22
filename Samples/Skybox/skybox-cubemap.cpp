#include "imgui.h"
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
	class SkyBoxPanoramic : public GLSampleWindow {
	  public:
		SkyBoxPanoramic() : GLSampleWindow() {
			this->setTitle("Skybox Cubemap");
			this->skyboxSettingComponent =
				std::make_shared<SkyboxPanoramicSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->skyboxSettingComponent);

			this->camera.enableNavigation(false);
		}

		GeometryObject SkyboxCube;

		unsigned int skybox_program;
		unsigned int skybox_cubemap;

		struct UniformBufferBlock {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor;
			float exposure = 1.0f;
			float gamma = 2.2f;
		} uniform_stage_buffer;

		CameraController camera;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformBufferBlock);

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/cubemap.frag.spv";

	  public:
		class SkyboxPanoramicSettingComponent : public nekomimi::UIComponent {

		  public:
			SkyboxPanoramicSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("SkyBox Settings");
			}
			void draw() override {
				ImGui::ColorEdit4("Tint", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Exposure", &this->uniform.exposure);
				ImGui::DragFloat("Gamma", &this->uniform.gamma);
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SkyboxPanoramicSettingComponent> skyboxSettingComponent;

		void Release() override {
			glDeleteProgram(this->skybox_program);
			glDeleteVertexArrays(1, &this->SkyboxCube.vao);
			glDeleteBuffers(1, &this->SkyboxCube.vbo);
			glDeleteTextures(1, (const GLuint *)&this->skybox_cubemap);
		}

		void Initialize() override {

			std::vector<std::string> cubemapPaths = {"asset/X+.png", "asset/X-.png", "asset/Y+.png",
													 "asset/Y-.png", "asset/Z+.png", "asset/Z-.png"};

			/*	Load shader binaries.	*/
			std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());
			/*	*/
			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Create skybox graphic pipeline program.	*/
			this->skybox_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			unsigned int uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "textureCubeMap"), 0);
			glUseProgram(0);

			/*	Load cubemap.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->skybox_cubemap = textureImporter.loadCubeMap(cubemapPaths);

			/*  */
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformSize = Math::align(uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1.0f, vertices, indices);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->SkyboxCube.vao);
			glBindVertexArray(this->SkyboxCube.vao);

			/*  */
			glGenBuffers(1, &this->SkyboxCube.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SkyboxCube.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->SkyboxCube.nrIndicesElements = indices.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->SkyboxCube.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, SkyboxCube.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex),
						 vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			glBindVertexArray(0);
		}

		void draw() override {

			int width, height;
			getSize(&width, &height);

			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
							  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize, this->uniformSize);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);

			/*  */
			glUseProgram(this->skybox_program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->skybox_cubemap);

			/*	Draw triangle*/
			glBindVertexArray(this->SkyboxCube.vao);
			glDrawElements(GL_TRIANGLES, this->SkyboxCube.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);
		}

		void update() override {
			/*	*/
			camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->uniform_stage_buffer.proj =
				glm::perspective(glm::radians(45.0f), (float)this->width() / (float)this->height(), 0.15f, 1000.0f);
			this->uniform_stage_buffer.modelViewProjection = (this->uniform_stage_buffer.proj * camera.getViewMatrix());

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformSize,
				this->uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
	class SkyBoxPanoramicGLSample : public GLSample<SkyBoxPanoramic> {
	  public:
		SkyBoxPanoramicGLSample() : GLSample<SkyBoxPanoramic>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path",
					cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SkyBoxPanoramicGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
