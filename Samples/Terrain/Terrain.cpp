#include "Common.h"
#include "GLSampleSession.h"
#include "PostProcessing/MistPostProcessing.h"
#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <Scene.h>
#include <ShaderLoader.h>
#include <Skybox.h>
#include <Util/CameraController.h>
#include <glm/fwd.hpp>
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

		struct alignas(16) TerrainSettings {
			glm::ivec2 size = glm::ivec2(1, 1);
			glm::vec2 tile_offset = glm::vec2(10, 10);

			glm::vec2 tile_noise_size = glm::vec2(10.000, 10.000);
			glm::vec2 tile_noise_offset = glm::vec2(0, 0);
		};
		struct alignas(16) WaterSettings {};

		struct alignas(16) UniformTerrainBufferBlock {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 viewProjection{};
			glm::mat4 modelViewProjection{};

			CameraInstance camera{};

			TerrainSettings terrainSettings;

			/*	Material	*/
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1.0f);
			glm::vec4 diffuseColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 shinines = glm::vec4(8, 1, 1, 1);

			/*	light source.	*/
			DirectionalLight directional;

			/*	Tessellation Settings.	*/
			glm::vec4 gEyeWorldPos{};
			float gDisplace = 1.0f;
			float tessLevel = 1;
			float maxTessellation = 30.0f;
			float minTessellation = 0.05f;
		};

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
		MeshObject ocean_water;
		MistPostProcessing mistprocessing;

		unsigned int terrain_program = 0;
		unsigned int simple_ocean_program = 0;

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

		unsigned int terrain_diffuse_texture = 0;
		unsigned int terrain_heightMap = 0;
		unsigned int irradiance_texture = 0;
		unsigned int color_texture = 0;
		unsigned int ocean_normal = 0;

		glm::mat4 cameraProj{};
		CameraController camera;

		/*	Simple Terrain.	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert.spv";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag.spv";
		const std::string vertexTerrainControlShaderPath = "Shaders/terrain/terrain.tesc.spv";
		const std::string fragmentTerrainEvolutionShaderPath = "Shaders/terrain/terrain.tese.spv";

		/*	Simple Water.	*/
		const std::string vertexSimpleWaterShaderPath = "Shaders/simpleocean/simple_water.vert.spv";
		const std::string fragmentSimpleWaterShaderPath = "Shaders/simpleocean/simple_water.frag.spv";

		/*	Occlusion.	*/

		void Release() override {
			glDeleteProgram(this->terrain_program);
			glDeleteProgram(this->simple_ocean_program);

			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			glDeleteVertexArrays(1, &this->ocean_water.vao);
			glDeleteBuffers(1, &this->ocean_water.vbo);
			glDeleteBuffers(1, &this->ocean_water.ibo);
		}

		class TerrainSettingComponent : public nekomimi::UIComponent {
		  public:
			TerrainSettingComponent(struct uniform_buffer_block &uniform) : stage_uniform(uniform) {
				this->setName("Terrain Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Setting");
				{
					ImGui::ColorEdit4("Color", &this->stage_uniform.terrain.directional.lightColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					if (ImGui::DragFloat3("Light Direction",
										  &this->stage_uniform.terrain.directional.lightDirection[0])) {
					}
				}
				ImGui::TextUnformatted("Material Setting");
				ImGui::ColorEdit4("Ambient", &this->stage_uniform.terrain.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Diffuse", &this->stage_uniform.terrain.diffuseColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->stage_uniform.terrain.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Shin", &this->stage_uniform.terrain.shinines[0]);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Displacement", &this->stage_uniform.terrain.gDisplace, 1, -1000.0f, 1000.0f);
				ImGui::DragFloat("Levels", &this->stage_uniform.terrain.tessLevel, 1, 0.0f, 32.0f);
				ImGui::DragFloat("Min Tessellation", &this->stage_uniform.terrain.minTessellation, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Max Tessellation", &this->stage_uniform.terrain.maxTessellation, 1, 0.0f, 100.0f);
				// ImGui::DragFloat("Distance Tessellation", &this->stage_uniform.terrain.maxTessellation, 1, 0.0f,
				// 				 100.0f);

				ImGui::TextUnformatted("Terrain Settings");
				ImGui::Checkbox("Show Terrain", &this->showTerrain);

				if (ImGui::DragFloat2("Tile Scale", &this->stage_uniform.terrain.terrainSettings.tile_offset[0])) {
					updateTerrain = true;
				}
				if (ImGui::DragFloat2("Tile Noise Scale",
									  &this->stage_uniform.terrain.terrainSettings.tile_noise_size[0])) {
					updateTerrain = true;
				}
				if (ImGui::DragFloat2("Tile Noise Offset",
									  &this->stage_uniform.terrain.terrainSettings.tile_noise_offset[0])) {
					updateTerrain = true;
				}
				ImGui::DragInt2("Size ", &this->stage_uniform.terrain.terrainSettings.size[0]);

				ImGui::TextUnformatted("Ocean Settings");
				ImGui::Checkbox("Show Ocean", &this->showOcean);
				// Depth

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Use MistFog", &this->useMistFogPost);
			}

			bool showWireFrame = false;
			bool showOcean = true;
			bool showTerrain = true;
			bool useMistFogPost = false;
			bool updateTerrain = false;

		  private:
			struct uniform_buffer_block &stage_uniform;
		};
		std::shared_ptr<TerrainSettingComponent> terrainSettingComponent;

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();

			{

				/*	*/
				const std::vector<uint32_t> vertex_terrain_binary =
					IOUtil::readFileData<uint32_t>(this->vertexTerrainShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_terrain_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentTerrainShaderPath, this->getFileSystem());
				const std::vector<uint32_t> control_binary_binary =
					IOUtil::readFileData<uint32_t>(this->vertexTerrainControlShaderPath, this->getFileSystem());
				const std::vector<uint32_t> evolution_terrain_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentTerrainEvolutionShaderPath, this->getFileSystem());

				const std::vector<uint32_t> vertex_simple_ocean_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSimpleWaterShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_simple_ocean_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSimpleWaterShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->terrain_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_terrain_binary, &fragment_terrain_binary,
													 nullptr, &control_binary_binary, &evolution_terrain_binary);

				this->simple_ocean_program = ShaderLoader::loadGraphicProgram(
					compilerOptions, &vertex_simple_ocean_binary, &fragment_simple_ocean_binary);
			}

			/*	Create Terrain Shader.	*/
			glUseProgram(this->terrain_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->terrain_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->terrain_program, "DiffuseTexture"), TextureType::Diffuse);
			glUniform1i(glGetUniformLocation(this->terrain_program, "NormalTexture"), TextureType::Normal);
			glUniform1i(glGetUniformLocation(this->terrain_program, "DisplacementTexture"), TextureType::Displacement);
			glUniform1i(glGetUniformLocation(this->terrain_program, "IrradianceTexture"), TextureType::Irradiance);
			glUniformBlockBinding(this->terrain_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->simple_ocean_program);
			glUniform1i(glGetUniformLocation(this->simple_ocean_program, "DepthTexture"), TextureType::DepthBuffer);
			glUniform1i(glGetUniformLocation(this->simple_ocean_program, "NormalTexture"), TextureType::Normal);
			glUniform1i(glGetUniformLocation(this->simple_ocean_program, "IrradianceTexture"), TextureType::Irradiance);
			glUniform1i(glGetUniformLocation(this->simple_ocean_program, "ReflectionTexture"), TextureType::Reflection);

			uniform_buffer_index = glGetUniformBlockIndex(this->simple_ocean_program, "UniformBufferBlock");
			glUniformBlockBinding(this->simple_ocean_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			TextureImporter textureImporter(this->getFileSystem());

			const int skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());
			/*	load Textures	*/
			const unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, skybox_program);

			/*	Create terrain texture.	*/
			this->terrain_diffuse_texture = textureImporter.loadImage2D(panoramicPath);
			this->color_texture = Common::createColorTexture(1, 1, Color(0, 1, 0, 1));

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

			ProcessData util = ProcessData(this->getFileSystem());

			/*	Generate HeightMap.	*/
			{
				const size_t noiseW = 2048;
				const size_t noiseH = 2048;

				/*	Create random texture.	*/
				util.computePerlinNoise(&this->terrain_heightMap, noiseW, noiseH);
			}

			/*	*/
			util.computeBump2Normal(this->terrain_heightMap, this->ocean_normal, 2048, 2048);

			/*	*/
			util.computeIrradiance(this->skybox.getTexture(), this->irradiance_texture, 256, 128);

			this->mistprocessing.initialize(this->getFileSystem());

			/*	Load geometry.	*/
			Common::loadPlan(this->terrain, 1, 32, 32);
			Common::loadPlan(this->ocean_water, 20, 1, 1);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			glDepthMask(GL_TRUE); //? Why needed?

			glClear(GL_DEPTH_BUFFER_BIT);

			if (this->terrainSettingComponent->updateTerrain) {
				this->terrainSettingComponent->updateTerrain = false;
				static ProcessData util = ProcessData(this->getFileSystem());
				util.computePerlinNoise(this->terrain_heightMap,
										this->uniform_stage_buffer.terrain.terrainSettings.tile_noise_size,
										this->uniform_stage_buffer.terrain.terrainSettings.tile_noise_offset);
				util.computeBump2Normal(this->terrain_heightMap, this->ocean_normal);
			}

			/*	Terrain.	*/
			if (this->terrainSettingComponent->showTerrain) {
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	Draw terrain.	*/
				glUseProgram(this->terrain_program);

				glEnable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glCullFace(GL_BACK);
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
				glDepthFunc(GL_LESS);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Diffuse);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
				glBindTexture(GL_TEXTURE_2D, this->ocean_normal);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Displacement);
				glBindTexture(GL_TEXTURE_2D, this->terrain_heightMap);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::DepthBuffer);
				glBindTexture(GL_TEXTURE_2D, this->getFrameBuffer()->depthbuffer);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Reflection);
				glBindTexture(GL_TEXTURE_2D, this->skybox.getTexture());

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				glBindVertexArray(this->terrain.vao);
				glPatchParameteri(GL_PATCH_VERTICES, 3);

				glDrawElements(GL_PATCHES, this->terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				/*	Render wireframe.	*/
				if (this->terrainSettingComponent->showWireFrame) {

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDepthFunc(GL_LEQUAL);

					/*	*/
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->color_texture);

					glPatchParameteri(GL_PATCH_VERTICES, 3);
					glDrawElements(GL_PATCHES, this->terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

					/*	Optional - to display wireframe.	*/
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				glBindVertexArray(0);
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);

			// Ocean/Water
			if (this->terrainSettingComponent->showOcean) {
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);
				/*	*/
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				glDepthFunc(GL_LESS);
				glDisable(GL_CULL_FACE);

				/*	Blending.	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::DepthBuffer);
				glBindTexture(GL_TEXTURE_2D, this->getFrameBuffer()->depthbuffer);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
				glBindTexture(GL_TEXTURE_2D, this->ocean_normal);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Reflection);
				glBindTexture(GL_TEXTURE_2D, this->skybox.getTexture());

				glUseProgram(this->simple_ocean_program);
				glBindVertexArray(this->ocean_water.vao);
				glDrawElements(GL_TRIANGLES, this->ocean_water.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				/*	Draw depth only to depth buffer.	*/
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				glDisable(GL_BLEND);
				glDepthMask(GL_TRUE);
				glDrawElements(GL_TRIANGLES, this->ocean_water.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

				glBindVertexArray(0);

				glUseProgram(0);
			}

			/*	Post processing.	*/
			if (this->terrainSettingComponent->useMistFogPost) {
				this->mistprocessing.render(this->irradiance_texture, this->getFrameBuffer()->attachments[0],
											this->getFrameBuffer()->depthbuffer);
			}
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update buffer.	*/
			this->uniform_stage_buffer.terrain.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.terrain.model =
				glm::rotate(this->uniform_stage_buffer.terrain.model, (float)-Math::PI_half, glm::vec3(1, 0, 0));
			this->uniform_stage_buffer.terrain.model =
				glm::scale(this->uniform_stage_buffer.terrain.model, glm::vec3(200.0f));

			this->uniform_stage_buffer.terrain.view = this->camera.getViewMatrix();
			this->uniform_stage_buffer.terrain.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.terrain.viewProjection =
				this->uniform_stage_buffer.terrain.proj * this->uniform_stage_buffer.terrain.view;

			this->uniform_stage_buffer.terrain.modelViewProjection = this->uniform_stage_buffer.terrain.proj *
																	 this->uniform_stage_buffer.terrain.view *
																	 this->uniform_stage_buffer.terrain.model;
			this->uniform_stage_buffer.terrain.gEyeWorldPos = glm::vec4(this->camera.getPosition(), 0);

			this->uniform_stage_buffer.terrain.camera.position = glm::vec4(this->camera.getPosition(), 0);
			this->uniform_stage_buffer.terrain.camera.viewDir = glm::vec4(this->camera.getLookDirection(), 0);

			/*	*/
			this->mistprocessing.mistsettings.proj = this->camera.getProjectionMatrix();
			this->mistprocessing.mistsettings.fogSettings.cameraNear = this->camera.getNear();
			this->mistprocessing.mistsettings.fogSettings.cameraFar = this->camera.getFar();
			this->mistprocessing.mistsettings.viewRotation = camera.getRotationMatrix();

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
					cxxopts::value<std::string>()->default_value("asset/industrial_sunset_puresky_4k.exr"));
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
