#include "Common.h"
#include "GLSampleSession.h"
#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Skybox.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class Terrain : public GLSampleWindow {
	  public:
		Terrain() : GLSampleWindow() {
			this->setTitle("Terrain");

			this->terrainSettingComponent = std::make_shared<TerrainSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->terrainSettingComponent);

			this->camera.setFar(2000.0f);
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformTerrainBufferBlock {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 viewProjection{};
			glm::mat4 modelViewProjection{};

			/*	Material	*/
			glm::vec4 diffuseColor{};
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);

			/*	light source.	*/
			glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

			/*	Fog settings.	*/
			FogSettings fogSettings;

			/*	Tessellation Settings.	*/
			glm::vec4 gEyeWorldPos{};
			float tessLevel = 1;
		};

		struct UniformLightBufferBlock {
			/*	light source.	*/
			glm::vec4 lookDirection{};
			glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position{};

		} uniformLight;

		using UniformOceanBufferBlock = struct UniformOceanBufferBlock_t {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Material	*/
			float shininess = 8;
			float fresnelPower = 4;
			glm::vec4 oceanColor = glm::vec4(0, 0.4, 1, 1);
		};

		/*	Pack all uniform in single buffer.	*/
		struct uniform_buffer_block {
			UniformTerrainBufferBlock terrain;
			// UniformOceanBufferBlock ocean;
		} uniform_stage_buffer;

		Skybox skybox;

		MeshObject terrain;
		MeshObject ocean_volume;

		unsigned int skybox_program{};
		unsigned int terrain_program{};
		unsigned int ocean_program{};

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_light_buffer_binding = 1;
		unsigned int uniform_buffer{};
		unsigned int uniform_light_buffer{};
		const size_t nrUniformBuffer = 3;

		/*	Uniform align buffer sizes.	*/
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t terrainUniformSize = 0;
		size_t oceanUniformSize = 0;

		unsigned int terrain_diffuse_texture{};
		unsigned int terrain_heightMap{};

		glm::mat4 cameraProj{};
		CameraController camera;

		/*	Simple Terrain.	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert.spv";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag.spv";
		const std::string vertexTerrainControlShaderPath = "Shaders/terrain/terrain.tesc.spv";
		const std::string fragmentTerrainEvolutionShaderPath = "Shaders/terrain/terrain.tese.spv";

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	Simple Water.	*/
		const std::string vertexSimpleWaterShaderPath = "Shaders/simpleocean/simpleocean.vert.spv";
		const std::string fragmentSimpleWaterShaderPath = "Shaders/simpleocean/simpleocean.frag.spv";

		/*	Occlusion.	*/

		void Release() override {
			glDeleteProgram(this->terrain_program);
			glDeleteProgram(this->ocean_program);
			glDeleteProgram(this->skybox_program);

			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			glDeleteVertexArrays(1, &this->ocean_volume.vao);
			glDeleteBuffers(1, &this->ocean_volume.vbo);
			glDeleteBuffers(1, &this->ocean_volume.ibo);
		}

		class TerrainSettingComponent : public nekomimi::UIComponent {
		  public:
			TerrainSettingComponent(struct uniform_buffer_block &uniform) : stage_uniform(uniform) {
				this->setName("Terrain Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Setting");
				{
					ImGui::ColorEdit4("Color", &this->stage_uniform.terrain.lightColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					if (ImGui::DragFloat3("Light Direction", &this->stage_uniform.terrain.lightDirection[0])) {
					}
				}
				ImGui::TextUnformatted("Material Setting");
				ImGui::ColorEdit4("Ambient", &this->stage_uniform.terrain.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Diffuse", &this->stage_uniform.terrain.diffuseColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->stage_uniform.terrain.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				ImGui::TextUnformatted("Fog Setting");
				ImGui::DragInt("Fog Type", (int *)&this->stage_uniform.terrain.fogSettings.fogType);
				ImGui::ColorEdit4("Fog Color", &this->stage_uniform.terrain.fogSettings.fogColor[0],
								  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				ImGui::DragFloat("Fog Density", &this->stage_uniform.terrain.fogSettings.fogDensity);
				ImGui::DragFloat("Fog Intensity", &this->stage_uniform.terrain.fogSettings.fogIntensity);
				ImGui::DragFloat("Fog Start", &this->stage_uniform.terrain.fogSettings.fogStart);
				ImGui::DragFloat("Fog End", &this->stage_uniform.terrain.fogSettings.fogEnd);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Levels", &this->stage_uniform.terrain.tessLevel, 1, 0.0f, 6.0f);

				ImGui::TextUnformatted("Ocean Settings");

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &stage_uniform;
		};
		std::shared_ptr<TerrainSettingComponent> terrainSettingComponent;

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();

			{

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				const std::vector<uint32_t> vertex_gouraud_binary =
					IOUtil::readFileData<uint32_t>(this->vertexTerrainShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentTerrainShaderPath, this->getFileSystem());
				const std::vector<uint32_t> control_binary =
					IOUtil::readFileData<uint32_t>(this->vertexTerrainControlShaderPath, this->getFileSystem());
				const std::vector<uint32_t> evolution_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentTerrainEvolutionShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->terrain_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_gouraud_binary, &fragment_binary, nullptr,
													 &control_binary, &evolution_binary);

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Create Terrain Shader.	*/
			glUseProgram(this->terrain_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->terrain_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->terrain_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->terrain_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->terrain_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			this->skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, this->skybox_program);

			/*	Create terrain texture.	*/
			this->terrain_diffuse_texture = textureImporter.loadImage2D(panoramicPath);

			/*	*/
			GLint minMapBufferSize = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->oceanUniformSize = Math::align<size_t>(this->uniformAlignBufferSize, (size_t)minMapBufferSize);
			this->uniformAlignBufferSize = Math::align<size_t>(this->oceanUniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformAlignBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Generate HeightMap.	*/
			{
				const size_t noiseW = 1024;
				const size_t noiseH = 1024;

				std::vector<float> heightMap(noiseW * noiseH);
				float noiseScale = 10.0f;
				for (size_t x = 0; x < noiseW; x++) {
					for (size_t y = 0; y < noiseH; y++) {

						float height = Math::PerlinNoise(x * noiseScale, y * noiseScale);
						heightMap[x * noiseW + y] = height;
					}
				}

				/*	Create random texture.	*/
				glGenTextures(1, &this->terrain_heightMap);
				glBindTexture(GL_TEXTURE_2D, this->terrain_heightMap);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, noiseW, noiseH, 0, GL_RED, GL_FLOAT, heightMap.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				/*	Border clamped to max value, it makes the outside area.	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);
				glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, 0);
			}

			/*	Load geometry.	*/
			{
				Common::loadCube(this->ocean_volume, 1);
				Common::loadPlan(this->terrain, 16, 16);

				/*	Update terrain height.	*/
				glBindBuffer(GL_ARRAY_BUFFER, this->terrain.vbo);
				ProceduralGeometry::Vertex *vertices = (ProceduralGeometry::Vertex *)glMapBufferRange(
					GL_ARRAY_BUFFER, 0, this->terrain.nrVertices * sizeof(vertices[0]),
					GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);

				/*	Create Terrain Mesh Displacement.	*/
				float noiseScale = 10.0f;
				for (size_t i = 0; i < terrain.nrVertices; i++) {
					float height = 0;
					height +=
						Math::PerlinNoise(vertices[i].vertex[0] * noiseScale, vertices[i].vertex[1] * noiseScale) *
						0.1f;
					height += Math::PerlinNoise(vertices[i].vertex[0] * 2, vertices[i].vertex[1] * 2) * 0.6;

					std::swap(vertices[i].vertex[1], vertices[i].vertex[2]);
					vertices[i].vertex[1] = height;
				}
				glUnmapBuffer(GL_ARRAY_BUFFER);
			}
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Terrain.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	Draw terrain.	*/
				glUseProgram(this->terrain_program);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, this->terrain_heightMap);

				/*	Irradiance.	*/

				glBindVertexArray(this->terrain.vao);
				glPatchParameteri(GL_PATCH_VERTICES, 3);

				if (!validateExistingProgram()) {
					std::cout << getProgramValidateString() << std::endl;
				}

				glDrawElements(GL_PATCHES, this->terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			// Ocean/Water
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	*/
				glEnable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);

				glUseProgram(this->ocean_program);

				glBindVertexArray(this->ocean_volume.vao);
				// glDrawElements(GL_TRIANGLES, ocean_volume.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update buffer.	*/
			this->uniform_stage_buffer.terrain.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.terrain.model =
				glm::scale(this->uniform_stage_buffer.terrain.model, glm::vec3(0.0025f));

			this->uniform_stage_buffer.terrain.view = this->camera.getViewMatrix();
			this->uniform_stage_buffer.terrain.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.terrain.viewProjection =
				this->uniform_stage_buffer.terrain.proj * this->uniform_stage_buffer.terrain.view;

			this->uniform_stage_buffer.terrain.modelViewProjection = this->uniform_stage_buffer.terrain.proj *
																	 this->uniform_stage_buffer.terrain.view *
																	 this->uniform_stage_buffer.terrain.model;
			this->uniform_stage_buffer.terrain.gEyeWorldPos = glm::vec4(this->camera.getPosition(), 0);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class TerrainGLSample : public GLSample<Terrain> {
	  public:
		TerrainGLSample() : GLSample<Terrain>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("S,skybox", "Skybox Texture File Path",
					cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::TerrainGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
