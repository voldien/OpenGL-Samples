#include "Common.h"
#include "GLSampleSession.h"
#include "Math3D/Color.h"
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
	class Planet : public GLSampleWindow {
	  public:
		Planet() : GLSampleWindow() {
			this->setTitle("Planet");

			/*	*/
			this->planetSettingComponent = std::make_shared<PlanetSettingComponent>(*this);
			this->addUIComponent(this->planetSettingComponent);

			/*	*/
			this->camera.setFar(2000.0f);
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct alignas(16) PlanetSettings {
			glm::ivec2 size = glm::ivec2(1, 1);
			glm::vec2 tile_offset = glm::vec2(10, 10);

			glm::vec2 tile_noise_size = glm::vec2(10.000, 10.000);
			glm::vec2 tile_noise_offset = glm::vec2(0, 0);
		};

		struct UniformPlanetBufferBlock {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 viewProjection{};
			glm::mat4 modelViewProjection{};

			CameraInstanceData camera{};

			PlanetSettings planetSettings;

			/*	Material	*/
			glm::vec4 ambientColor = glm::vec4(0.2, 0.2, 0.2, 1.0f);
			glm::vec4 diffuseColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 specularColor = glm::vec4(1, 1, 1, 1);
			glm::vec4 shinines = glm::vec4(8, 1, 1, 1);

			/*	light source.	*/
			DirectionalLightData directional;

			/*	Tessellation Settings.	*/
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
		struct uniform_buffer_block_t {
			UniformPlanetBufferBlock planet;
		} uniform_stage_buffer;

		Skybox skybox;

		MeshObject planet_sphere;

		unsigned int planet_program = 0;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_light_buffer_binding = 1;
		unsigned int uniform_buffer{};
		unsigned int uniform_light_buffer{};
		const size_t nrUniformBuffer = 3;

		/*	Uniform align buffer sizes.	*/
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block_t);
		size_t planetUniformSize = 0;
		size_t oceanUniformSize = 0;

		unsigned int planet_diffuse_texture = 0;
		unsigned int planet_heightMap = 0;
		unsigned int planet_noise_textures = 0;
		unsigned int irradiance_texture = 0;
		unsigned int color_texture = 0;
		unsigned int planet_normal = 0;

		CameraController camera;

		/*	Simple Planet.	*/
		const std::string vertexPlanetShaderPath = "Shaders/planet/planet.vert.spv";
		const std::string fragmentPlanetShaderPath = "Shaders/planet/planet.frag.spv";
		const std::string vertexPlanetControlShaderPath = "Shaders/planet/planet.tesc.spv";
		const std::string fragmentPlanetEvolutionShaderPath = "Shaders/planet/planet.tese.spv";

		void Release() override {
			glDeleteProgram(this->planet_program);

			glDeleteVertexArrays(1, &this->planet_sphere.vao);
			glDeleteBuffers(1, &this->planet_sphere.vbo);
			glDeleteBuffers(1, &this->planet_sphere.ibo);
		}

		class PlanetSettingComponent : public GLUIComponent<Planet> {
		  public:
			PlanetSettingComponent(Planet &sample)
				: GLUIComponent(sample, "Planet Settings"), stage_uniform(this->getRefSample().uniform_stage_buffer) {}

			void draw() override {

				ImGui::TextUnformatted("Light Setting");
				{
					ImGui::ColorEdit4("Color", &this->stage_uniform.planet.directional.lightColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
					if (ImGui::DragFloat3("Light Direction",
										  &this->stage_uniform.planet.directional.lightDirection[0])) {
					}
				}
				ImGui::TextUnformatted("Material Setting");
				ImGui::ColorEdit4("Ambient", &this->stage_uniform.planet.ambientColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Diffuse", &this->stage_uniform.planet.diffuseColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Specular Color", &this->stage_uniform.planet.specularColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Shin", &this->stage_uniform.planet.shinines[0]);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Displacement", &this->stage_uniform.planet.gDisplace, 1, -1000.0f, 1000.0f);
				ImGui::DragFloat("Levels", &this->stage_uniform.planet.tessLevel, 1, 0.0f, 32.0f);
				ImGui::DragFloat("Min Tessellation", &this->stage_uniform.planet.minTessellation, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Max Tessellation", &this->stage_uniform.planet.maxTessellation, 1, 0.0f, 100.0f);
				// ImGui::DragFloat("Distance Tessellation", &this->stage_uniform.planet.maxTessellation, 1, 0.0f,
				// 				 100.0f);

				ImGui::TextUnformatted("Planet Settings");
				ImGui::Checkbox("Show Planet", &this->showPlanet);

				if (ImGui::DragFloat2("Tile Scale", &this->stage_uniform.planet.planetSettings.tile_offset[0])) {
					updatePlanet = true;
				}
				if (ImGui::DragFloat2("Tile Noise Scale",
									  &this->stage_uniform.planet.planetSettings.tile_noise_size[0])) {
					updatePlanet = true;
				}
				if (ImGui::DragFloat2("Tile Noise Offset",
									  &this->stage_uniform.planet.planetSettings.tile_noise_offset[0])) {
					updatePlanet = true;
				}
				ImGui::DragInt2("Size ", &this->stage_uniform.planet.planetSettings.size[0]);

				// Depth

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;
			bool showPlanet = true;
			bool useMistFogPost = false;
			bool updatePlanet = false;

		  private:
			struct uniform_buffer_block_t &stage_uniform;
		};
		std::shared_ptr<PlanetSettingComponent> planetSettingComponent;

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox"].as<std::string>();
			const std::string planetDiffusePath = this->getResult()["planet-texture"].as<std::string>();

			{

				/*	*/
				const std::vector<uint32_t> vertex_planet_binary =
					IOUtil::readFileData<uint32_t>(this->vertexPlanetShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_planet_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentPlanetShaderPath, this->getFileSystem());
				const std::vector<uint32_t> control_binary_binary =
					IOUtil::readFileData<uint32_t>(this->vertexPlanetControlShaderPath, this->getFileSystem());
				const std::vector<uint32_t> evolution_planet_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentPlanetEvolutionShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				this->planet_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_planet_binary, &fragment_planet_binary,
													 nullptr, &control_binary_binary, &evolution_planet_binary);
			}

			/*	Create Planet Shader.	*/
			glUseProgram(this->planet_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->planet_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->planet_program, "DiffuseTexture"), TextureType::Diffuse);
			glUniform1i(glGetUniformLocation(this->planet_program, "NormalTexture"), TextureType::Normal);
			glUniform1i(glGetUniformLocation(this->planet_program, "DisplacementTexture"), TextureType::Displacement);
			glUniform1i(glGetUniformLocation(this->planet_program, "IrradianceTexture"), TextureType::Irradiance);
			glUniformBlockBinding(this->planet_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			TextureImporter textureImporter(this->getFileSystem());

			const int skybox_program = Skybox::loadDefaultProgram(this->getFileSystem());
			/*	load Textures	*/
			const unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, skybox_program);

			/*	Load planet texture.	*/
			this->planet_diffuse_texture = textureImporter.loadImage2D(planetDiffusePath);

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

			MiscProcessingUtil util = MiscProcessingUtil(this->getFileSystem());

			/*	Generate HeightMap.	*/
			// TODO: generate multiple layers.
			{
				const size_t noiseW = 2048;
				const size_t noiseH = 2048;
				/*	Create random texture.	*/

				util.computePerlinNoise(&this->planet_noise_textures, noiseW, noiseH);
			}

			util.computeColor2HeightMap(this->planet_diffuse_texture, this->planet_heightMap, 2048, 2048);

			/*	*/
			util.computeBump2Normal(this->planet_heightMap, this->planet_normal, 2048, 2048);

			/*	*/
			util.computeDiffuseIrradiance(this->skybox.getTexture(), this->irradiance_texture, 256, 128);

			this->color_texture = CommonUtil::createColorTexture(1, 1, fragcore::Color::red());

			/*	Load geometry.	*/
			CommonUtil::loadSphere(this->planet_sphere, 1, 32, 32);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			size_t width = 0, height = 0;
			this->getCurrentFrameBufferSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			glDepthMask(GL_TRUE); //? Why needed?
			glClear(GL_DEPTH_BUFFER_BIT);

			if (this->planetSettingComponent->updatePlanet) {
				this->planetSettingComponent->updatePlanet = false;
				static MiscProcessingUtil util = MiscProcessingUtil(this->getFileSystem());
				util.computePerlinNoise(this->planet_heightMap,
										this->uniform_stage_buffer.planet.planetSettings.tile_noise_size,
										this->uniform_stage_buffer.planet.planetSettings.tile_noise_offset);
				// util.computeBump2Normal(this->planet_heightMap, this->ocean_normal);
			}

			/*	Planet.	*/
			if (this->planetSettingComponent->showPlanet) {
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				/*	Draw planet.	*/
				glUseProgram(this->planet_program);

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
				glBindTexture(GL_TEXTURE_2D, this->planet_diffuse_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Normal);
				glBindTexture(GL_TEXTURE_2D, this->planet_normal);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Displacement);
				glBindTexture(GL_TEXTURE_2D, this->planet_heightMap);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::DepthBuffer);
				glBindTexture(GL_TEXTURE_2D, this->getDefaultFrameBufferObj()->attachments[this->getDefaultFrameBufferObj()->depthIndex]);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Irradiance);
				glBindTexture(GL_TEXTURE_2D, this->irradiance_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + TextureType::Reflection);
				glBindTexture(GL_TEXTURE_2D, this->skybox.getTexture());

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

				glBindVertexArray(this->planet_sphere.vao);
				glPatchParameteri(GL_PATCH_VERTICES, 3);

				glDrawElements(GL_PATCHES, this->planet_sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

				/*	Render wireframe.	*/
				if (this->planetSettingComponent->showWireFrame) {

					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
					glDepthFunc(GL_LEQUAL);

					/*	*/
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, this->color_texture);

					glPatchParameteri(GL_PATCH_VERTICES, 3);
					glDrawElements(GL_PATCHES, this->planet_sphere.nrIndicesElements, GL_UNSIGNED_INT, nullptr);

					/*	Optional - to display wireframe.	*/
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}

				glBindVertexArray(0);
				glUseProgram(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			this->camera.update(this->getTimer().deltaTime<float>());

			/*	Update buffer.	*/
			this->uniform_stage_buffer.planet.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.planet.model =
				glm::rotate(this->uniform_stage_buffer.planet.model, (float)-Math::PI_half, glm::vec3(1, 0, 0));
			this->uniform_stage_buffer.planet.model =
				glm::scale(this->uniform_stage_buffer.planet.model, glm::vec3(250.0f));

			this->uniform_stage_buffer.planet.view = this->camera.getViewMatrix();
			this->uniform_stage_buffer.planet.proj = this->camera.getProjectionMatrix();
			this->uniform_stage_buffer.planet.viewProjection =
				this->uniform_stage_buffer.planet.proj * this->uniform_stage_buffer.planet.view;

			this->uniform_stage_buffer.planet.modelViewProjection = this->uniform_stage_buffer.planet.proj *
																	 this->uniform_stage_buffer.planet.view *
																	 this->uniform_stage_buffer.planet.model;
			this->uniform_stage_buffer.planet.camera = this->camera;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class PlanetGLSample : public GLSample<Planet> {
	  public:
		PlanetGLSample() : GLSample<Planet>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("S,skybox", "Skybox Texture File Path",
					cxxopts::value<std::string>()->default_value("asset/NightSkyHDRI002_8K-HDR.exr"))(
				"P,planet-texture", "Planet Diffuse Texture",
				cxxopts::value<std::string>()->default_value("asset/8k_mars.jpg"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::PlanetGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
