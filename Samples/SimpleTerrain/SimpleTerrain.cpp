#include "GLSampleSession.h"
#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	// TODO rename to simple terrain.
	/**
	 * @brief
	 *
	 */
	class SimpleTerrain : public GLSampleWindow {
	  public:
		SimpleTerrain() : GLSampleWindow() {
			this->setTitle("Terrain");

			this->simpleOceanSettingComponent =
				std::make_shared<SimpleOceanSettingComponent>(this->uniform_stage_buffer);
			this->addUIComponent(this->simpleOceanSettingComponent);

			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformSkyBoxBufferBlock {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			float exposure = 1.0f;
			float gamma = 2.2;
		};

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

		struct UniformOceanBufferBlock {
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
			UniformSkyBoxBufferBlock skybox;
			UniformSkyBoxBufferBlock ocean;
		} uniform_stage_buffer;

		MeshObject skybox;
		MeshObject terrain;
		MeshObject plan;

		unsigned int skybox_program;
		unsigned int terrain_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_light_buffer_binding = 1;
		unsigned int uniform_buffer;
		unsigned int uniform_light_buffer;
		const size_t nrUniformBuffer = 3;

		/*	Uniform align buffer sizes.	*/
		size_t uniformBufferSize = sizeof(uniform_buffer_block);
		size_t skyboxUniformSize = 0;
		size_t terrainUniformSize = 0;
		size_t oceanUniformSize = 0;
		size_t lightUniformSize = 0;

		int terrain_diffuse_texture;
		int terrain_heightMap;

		glm::mat4 cameraProj;
		CameraController camera;

		/*	Simple Terrain.	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert.spv";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag.spv";

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		/*	Simple Water.	*/
		const std::string vertexSimpleWaterShaderPath = "Shaders/simpleocean/simpleocean.vert.spv";
		const std::string fragmentSimpleWaterShaderPath = "Shaders/simpleocean/simpleocean.frag.spv";

		void Release() override {
			glDeleteProgram(this->terrain_program);

			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			// glDeleteBuffers(1, &this->skybox_vbo);
			// glDeleteTextures(1, &this->skybox_panoramic);
		}

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {
		  public:
			SimpleOceanSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
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

				{
					/*	*/
					ImGui::TextUnformatted("Skybox");
					ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				}

				{
					/*	*/
					ImGui::TextUnformatted("Ocean");
					ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
									  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
					ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				}

				/*	*/
				ImGui::TextUnformatted("Debug");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleOceanSettingComponent;

		void Initialize() override {

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			{

				/*	Load shader	*/
				const std::vector<char> simple_terrain_vertex_source =
					IOUtil::readFileString(vertexTerrainShaderPath, this->getFileSystem());
				const std::vector<char> simple_terrain_fragment_source =
					IOUtil::readFileString(fragmentTerrainShaderPath, this->getFileSystem());

				this->terrain_program =
					ShaderLoader::loadGraphicProgram(&simple_terrain_vertex_source, &simple_terrain_fragment_source);
			}
			/*	Create Terrain Shader.	*/
			glUseProgram(this->terrain_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->terrain_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->terrain_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->terrain_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->terrain_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	Create terrain texture.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->terrain_diffuse_texture = textureImporter.loadImage2D(panoramicPath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->oceanUniformSize = Math::align(sizeof(UniformOceanBufferBlock), (size_t)minMapBufferSize);
			this->skyboxUniformSize = Math::align(sizeof(UniformSkyBoxBufferBlock), (size_t)minMapBufferSize);
			this->uniformBufferSize =
				Math::align(this->skyboxUniformSize + this->oceanUniformSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			{
				/*	Load geometry.	*/
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
			{ /*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(1, vertices, indices, 1, 1);
			}
			//		ProceduralGeometry::generateCube(1, vertices, indices);
		}

		void draw() override {

			this->update();
			int width, height;
			this->getSize(&width, &height);

			this->uniform_stage_buffer.terrain.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 10000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	*/
			{

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, uniform_buffer,
								  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				/*	Draw Skybox.	*/

				glDisable(GL_CULL_FACE);

				/*	Draw terrain.	*/
				glUseProgram(this->terrain_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				glBindVertexArray(this->terrain.vao);
				glDrawElements(GL_TRIANGLES, terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	*/
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize + 0,
								  this->skyboxUniformSize);

				glUseProgram(this->skybox_program);

				glDisable(GL_CULL_FACE);
				glDisable(GL_BLEND);
				glEnable(GL_DEPTH_TEST);
				glStencilMask(GL_FALSE);
				glDepthFunc(GL_LEQUAL);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glStencilMask(GL_TRUE);

				glUseProgram(0);
			}
			// glEnable(GL_DEPTH_TEST);
			// TODO disable depth write.
			// glDisable(GL_BLEND);
			// glEnable(GL_STENCIL);
			//
			///*	Draw Skybox.	*/
			// glUseProgram(this->skybox_program);
			//
			// glActiveTexture(GL_TEXTURE0);
			// glBindTexture(GL_TEXTURE_2D, this->terrain_diffuse_texture);
			//
			// glBindVertexArray(this->terrain_vao);
			// glDrawElements(GL_TRIANGLES, this->nrElements, GL_UNSIGNED_INT, nullptr);
			// glBindVertexArray(0);
		}

		void update() override {
			/*	Update Camera.	*/
			const float elapsedTime = getTimer().getElapsed<float>();
			camera.update(getTimer().deltaTime<float>());

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
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniform_stage_buffer, sizeof(this->uniform_stage_buffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class TerrainGLSample : public GLSample<SimpleTerrain> {
	  public:
		TerrainGLSample() : GLSample<SimpleTerrain>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("texture.png"));
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
