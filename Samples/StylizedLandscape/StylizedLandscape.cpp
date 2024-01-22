#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	class StylizedLandscape : public GLSampleWindow {
	  public:
		StylizedLandscape() : GLSampleWindow() {
			this->setTitle("Grass");
			this->grassSettingComponent = std::make_shared<GrassSettingComponent>(this->uniformGrassStageBuffer);
			this->addUIComponent(this->grassSettingComponent);
		}

		/*	*/
		struct UniformSkyBoxBufferBlock {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
		};

		/*	*/
		struct UniformGrassBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*light source.	*/
			glm::vec4 direction = glm::vec4(-1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			/*	Tessellation settings.	*/
			glm::vec3 eyePos = glm::vec3(1.0f);
			float gDisplace = 1.0f;
			float tessLevel = 1.0f;

		} uniformGrassStageBuffer;

		struct UniformTerrainBufferBlock {};

		struct UniformBufferBlock {
			UniformSkyBoxBufferBlock skybox;
			UniformGrassBufferBlock grass;
			UniformTerrainBufferBlock terrain;

		} uniformStageBuffer;

		class GrassSettingComponent : public nekomimi::UIComponent {

		  public:
			GrassSettingComponent(struct UniformGrassBufferBlock &uniform) : uniform(uniform) {
				this->setName("Grass Settings");
			}

			void draw() override {
				ImGui::DragFloat("Displacement", &this->uniform.gDisplace, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Tessellation Levels", &this->uniform.tessLevel, 1, 0.0f, 10.0f);
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0], ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0], ImGuiColorEditFlags_Float);

				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformGrassBufferBlock &uniform;
		};
		std::shared_ptr<GrassSettingComponent> grassSettingComponent;

		// Indirect drawing structure
		typedef struct indirect_buffer_t {
			std::uint32_t vertexCount = 5;
			std::uint32_t instanceCount = 1;
			std::uint32_t firstVertex = 0;
			std::uint32_t firstInstance = 0;
		} IndirectBuffer;

		/*	*/
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformSize = sizeof(UniformGrassBufferBlock);
		unsigned int grass_indirect_buffer = 0;
		/*	*/
		GeometryObject skybox;
		GeometryObject terrain;
		GeometryObject grass;
		GeometryObject water;

		/*	*/
		unsigned int terrain_program;
		unsigned int grass_program;
		unsigned int water_program;
		unsigned int skybox_program;

		CameraController camera;

		/*	*/
		unsigned int diffuse_texture;
		unsigned int heightmap_texture;
		unsigned int skybox_texture_panoramic;

		/*	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag";

		/*	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	*/
		const std::string vertexShaderPath = "Shaders/grass/grass.vert";
		const std::string fragmentShaderPath = "Shaders/grass/grass.frag";
		const std::string ControlShaderPath = "Shaders/grass/grass.tesc";
		const std::string EvoluationShaderPath = "Shaders/grass/grass.tese";

		void Release() override {

			glDeleteProgram(this->terrain_program);
			glDeleteProgram(this->grass_program);
			glDeleteProgram(this->water_program);
			glDeleteProgram(this->skybox_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->heightmap_texture);
			glDeleteTextures(1, (const GLuint *)&this->skybox_texture_panoramic);

			/*	*/
			glDeleteVertexArrays(1, &this->skybox.vao);
			glDeleteBuffers(1, &this->skybox.vbo);
			glDeleteBuffers(1, &this->skybox.ibo);
			glDeleteBuffers(1, &this->uniform_buffer);
		}

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			/*	*/
			const std::string diffuseTexturePath = "asset/tessellation_diffusemap.png";
			const std::string heightTexturePath = "asset/tessellation_heightmap.png";
			const std::string reflectionTexturePath = "asset/tessellation_heightmap.png";

			/*	*/
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());
			const std::vector<uint32_t> control_source =
				IOUtil::readFileData<uint32_t>(this->ControlShaderPath, this->getFileSystem());
			const std::vector<uint32_t> evolution_source =
				IOUtil::readFileData<uint32_t>(this->EvoluationShaderPath, this->getFileSystem());

			const std::vector<uint32_t> vertex_skybox_binary =
				IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_skybox_binary =
				IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader	*/
			this->grass_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source,
																   nullptr, &control_source, &evolution_source);

			this->skybox_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

			/*	Setup Shader.	*/
			glUseProgram(this->skybox_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->skybox_program, "diffuse"), 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "gDisplacementMap"), 1);
			glUniformBlockBinding(this->skybox_program, this->uniform_buffer_index, 0);
			glUseProgram(0);

			/*	Load Diffuse and Height Map Texture.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->diffuse_texture = textureImporter.loadImage2D(diffuseTexturePath);
			this->heightmap_texture = textureImporter.loadImage2D(heightTexturePath);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generateCube(1, vertices, indices);

			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformSize = Math::align(uniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	*/
			IndirectBuffer grass_indirect;
			glGenBuffers(1, &this->grass_indirect_buffer);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->grass_indirect_buffer);
			glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectBuffer), &grass_indirect, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vao);
			glBindVertexArray(this->skybox.vao);

			/*	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
						 GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indices.size();

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
			this->uniformGrassStageBuffer.proj =
				glm::perspective(glm::radians(45.0f), (float)width() / (float)height(), 0.15f, 1000.0f);

			int width, height;
			this->getSize(&width, &height);

			/*	Update grass culling.	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);
				glUseProgram(this->grass_program);

				int blades_count_ = 0;
				glDispatchCompute(static_cast<GLuint>(blades_count_), 1, 1);
			}

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformSize, this->uniformSize);

				glUseProgram(this->terrain_program);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->heightmap_texture);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				/*	Draw triangle*/
				glBindVertexArray(this->skybox.vao);
				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->grassSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				glPatchParameteri(GL_PATCH_VERTICES, 3);
				glDrawElements(GL_PATCHES, skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
			}
			/*	*/
			{ glUseProgram(this->water_program); }
			/*	*/
			{ glUseProgram(this->grass_program); }
			/*	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformSize,
								  this->uniformSize);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);

				/*	*/
				glUseProgram(this->skybox_program);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->skybox_texture_panoramic);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}
			/*	Draw wireframe outline.	*/
			// if (this->tessellationSettingComponent->showWireFrame) {
			//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			//	glDrawElements(GL_PATCHES, nrElements, GL_UNSIGNED_INT, nullptr);
			//}
		}

		void update() override {
			/*	*/
			const float elapsedTime = this->getTimer().getElapsed<float>();
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	*/
			this->uniformGrassStageBuffer.model = glm::mat4(1.0f);
			this->uniformGrassStageBuffer.model =
				glm::translate(this->uniformGrassStageBuffer.model, glm::vec3(0, 0, 10));
			this->uniformGrassStageBuffer.model =
				glm::rotate(this->uniformGrassStageBuffer.model, (float)Math::PI_half, glm::vec3(1, 0, 0));
			this->uniformGrassStageBuffer.model =
				glm::scale(this->uniformGrassStageBuffer.model, glm::vec3(10, 10, 10));

			/*	*/
			this->uniformGrassStageBuffer.view = camera.getViewMatrix();
			this->uniformGrassStageBuffer.modelViewProjection = this->uniformGrassStageBuffer.proj *
																this->uniformGrassStageBuffer.view *
																this->uniformGrassStageBuffer.model;
			this->uniformGrassStageBuffer.eyePos = camera.getPosition();

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer =
				glMapBufferRange(GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % nrUniformBuffer) * this->uniformSize,
								 uniformSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniformGrassStageBuffer, sizeof(uniformGrassStageBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};
	class GrassGLSample : public GLSample<StylizedLandscape> {
	  public:
		GrassGLSample() : GLSample<StylizedLandscape>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			// 			const std::string diffuseTexturePath = "asset/tessellation_diffusemap.png";
			// const std::string heightTexturePath = "asset/tessellation_heightmap.png";
			// const std::string reflectionTexturePath = "asset/tessellation_heightmap.png";
			options("T,skybox-texture", "Texture Path",
					cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::GrassGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}