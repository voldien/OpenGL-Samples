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

	// TODO rename to simple terrain.
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

			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformTerrainBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	Fog.	*/
			glm::vec4 fogColor = glm::vec4(1, 0, 0, 1);
			float cameraNear = 0.15f;
			float cameraFar = 1000.0f;
			float fogStart = 100;
			float fogEnd = 1000;
			float fogDensity = 0.1f;
			// FogType fogType = FogType::Exp;
			float fogIntensity = 1.0f;
		};

		struct UniformLightBufferBlock {
			/*	light source.	*/
			glm::vec4 lookDirection;
			glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 specularColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position;

		} uniformLight;

		typedef struct UniformOceanBufferBlock_t {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	Material	*/
			float shininess = 8;
			float fresnelPower = 4;
			glm::vec4 oceanColor = glm::vec4(0, 0.4, 1, 1);
		} UniformOceanBufferBlock;

		/*	Pack all uniform in single buffer.	*/
		struct uniform_buffer_block {
			UniformTerrainBufferBlock terrain;
			UniformOceanBufferBlock ocean;
		} uniform_stage_buffer;

		Skybox skybox;

		MeshObject terrain;
		MeshObject ocean_plan;

		unsigned int skybox_program;
		unsigned int terrain_program;
		unsigned int ocean_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_light_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_light_buffer;
		const size_t nrUniformBuffer = 3;

		/*	Uniform align buffer sizes.	*/
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);
		size_t terrainUniformSize = 0;
		size_t oceanUniformSize = 0;
		size_t lightUniformSize = 0;

		unsigned int terrain_diffuse_texture;
		unsigned int terrain_heightMap;

		glm::mat4 cameraProj;
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

		void Release() override {
			glDeleteProgram(this->terrain_program);
			glDeleteProgram(this->ocean_program);
			glDeleteProgram(this->skybox_program);

			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			glDeleteVertexArrays(1, &this->ocean_plan.vao);
			glDeleteBuffers(1, &this->ocean_plan.vbo);
			glDeleteBuffers(1, &this->ocean_plan.ibo);
		}

		class TerrainSettingComponent : public nekomimi::UIComponent {
		  public:
			TerrainSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Terrain Settings");
			}

			void draw() override {

				ImGui::TextUnformatted("Light Setting");
				{
					ImGui::ColorEdit4("Color", &this->uniform.terrain.lightColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					ImGui::ColorEdit4("Ambient", &this->uniform.terrain.ambientLight[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					// ImGui::ColorEdit4("Specular Color", &this->uniform.ocean.specularColor[0],
					//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					//
					// if (ImGui::DragFloat3("Light Direction", &this->uniform.ocean.lightDirection[0])) {
					//	this->uniform.ocean.lightDirection = glm::normalize(this->uniform.ocean.lightDirection);
					//}
				}

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<TerrainSettingComponent> terrainSettingComponent;

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

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

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			unsigned int skytexture = textureImporter.loadImage2D(panoramicPath);
			skybox.Init(skytexture, this->skybox_program);

			/*	Create terrain texture.	*/
			this->terrain_diffuse_texture = textureImporter.loadImage2D(panoramicPath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->oceanUniformSize = Math::align(sizeof(UniformOceanBufferBlock), (size_t)minMapBufferSize);
			this->uniformAlignBufferSize = Math::align(this->oceanUniformSize, (size_t)minMapBufferSize);

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
						heightMap[x * noiseW + y];
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

				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices, 2048, 2048);

				/*	Create Terrain Mesh Displacement.	*/
				float noiseScale = 10.0f;
				for (size_t i = 0; i < vertices.size(); i++) {
					float height = 0;
					for (size_t x = 0; x < 3; x++) {
						height +=
							Math::PerlinNoise(vertices[i].vertex[0] * noiseScale, vertices[i].vertex[1] * noiseScale) *
							0.1f;
						height += Math::PerlinNoise(vertices[i].vertex[0] * 2, vertices[i].vertex[1] * 2) * 0.6;
					}

					vertices[i].vertex[2] = height;
				}

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->terrain.vao);
				glBindVertexArray(this->terrain.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->terrain.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
							 GL_STATIC_DRAW);

				/*	*/
				glGenBuffers(1, &this->terrain.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(),
							 GL_STATIC_DRAW);
				this->terrain.nrIndicesElements = indices.size();

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

			/*	Load geometry.	*/
			Common::loadPlan(this->ocean_plan, 1);
		}

		void onResize(int width, int height) override { this->camera.setAspect((float)width / (float)height); }

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
								  (getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glDisable(GL_CULL_FACE);

				/*	Draw terrain.	*/
				glUseProgram(this->terrain_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, this->terrain_heightMap);

				glBindVertexArray(this->terrain.vao);
				glDrawElements(GL_TRIANGLES, terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			// Ocean
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
								  (getFrameCount() % nrUniformBuffer) * this->uniformAlignBufferSize,
								  this->uniformAlignBufferSize);

				glDisable(GL_CULL_FACE);
				glEnable(GL_BLEND);

				/*	Draw terrain.	*/
				glUseProgram(this->ocean_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				glBindVertexArray(this->terrain.vao);
				glDrawElements(GL_TRIANGLES, terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = getTimer().getElapsed<float>();
			this->camera.update(getTimer().deltaTime<float>());

			/*	*/
			this->uniform_stage_buffer.terrain.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.terrain.model =
				glm::rotate(this->uniform_stage_buffer.terrain.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniform_stage_buffer.terrain.model =
				glm::scale(this->uniform_stage_buffer.terrain.model, glm::vec3(1000.95f));
			this->uniform_stage_buffer.terrain.view = this->camera.getViewMatrix();
			this->uniform_stage_buffer.terrain.modelViewProjection = this->uniform_stage_buffer.terrain.proj *
																	 this->uniform_stage_buffer.terrain.view *
																	 this->uniform_stage_buffer.terrain.model;

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformAlignBufferSize,
				this->uniformAlignBufferSize,
				GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class TerrainGLSample : public GLSample<Terrain> {
	  public:
		TerrainGLSample() : GLSample<Terrain>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,skybox-texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
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
