#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <Importer/ImageImport.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	// TODO rename to simple terrain.
	class Terrain : public GLSampleWindow {
	  public:
		Terrain() : GLSampleWindow() {
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
		};
		struct UniformTerrainBufferBlock {
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
			alignas(16) glm::mat4 modelView;
			alignas(16) glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
		};

		/*	Pack all uniform in single buffer.	*/
		struct UniformBufferBlock {
			UniformTerrainBufferBlock terrain;
			UniformSkyBoxBufferBlock skybox;
		} uniform_stage_buffer;

		GeometryObject skybox;
		GeometryObject terrain;

		unsigned int skybox_program;
		unsigned int terrain_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_index;
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		/*	Uniform align buffer sizes.	*/
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t skyboxUniformSize = 0;
		size_t oceanUniformSize = 0;

		int terrain_diffuse_texture;
		int terrain_heightMap;
		std::string panoramicPath = "asset/panoramic.jpg";

		glm::mat4 proj;
		CameraController camera;

		/*	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag";

		virtual void Release() override {
			glDeleteProgram(this->terrain_program);

			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			// glDeleteBuffers(1, &this->skybox_vbo);
			// glDeleteTextures(1, &this->skybox_panoramic);
		}

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Simple Ocean Settings");
			}
			virtual void draw() override {
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Color", &this->uniform.terrain.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.terrain.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				/*	*/
				ImGui::TextUnformatted("Skybox");
				ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				ImGui::TextUnformatted("Debug");
				/*	*/
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleOceanSettingComponent;

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader	*/
			std::vector<char> vertex_source = IOUtil::readFileString(vertexTerrainShaderPath, this->getFileSystem());
			std::vector<char> fragment_source =
				IOUtil::readFileString(fragmentTerrainShaderPath, this->getFileSystem());

			this->terrain_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	Create Terrain Shade.	*/
			glUseProgram(this->terrain_program);
			this->uniform_buffer_index = glGetUniformBlockIndex(this->terrain_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->terrain_program, "DiffuseTexture"), 0);
			glUniform1i(glGetUniformLocation(this->terrain_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->terrain_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Create terrain texture.	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->terrain_diffuse_texture = textureImporter.loadImage2D(this->panoramicPath);

			/*	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			uniformBufferSize = Math::align(uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, 2048, 2048);

			/*	Create height offset.	*/
			for (size_t i = 0; i < vertices.size(); i++) {
				float height = Math::PerlinNoise(vertices[i].vertex[0] * 1.5f, vertices[i].vertex[1] * 1.5f);
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
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
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

			//		ProceduralGeometry::generateCube(1, vertices, indices);
		}

		virtual void draw() override {

			update();
			int width, height;
			getSize(&width, &height);

			this->uniform_stage_buffer.terrain.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, uniform_buffer,
							  (getFrameCount() % nrUniformBuffer) * this->uniformBufferSize, this->uniformBufferSize);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			{

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
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_index, this->uniform_buffer,
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

		virtual void update() override {
			/*	Update Camera.	*/
			float elapsedTime = getTimer().getElapsed();
			camera.update(getTimer().deltaTime());

			/*	*/
			this->uniform_stage_buffer.terrain.model = glm::mat4(1.0f);
			this->uniform_stage_buffer.terrain.model =
				glm::rotate(this->uniform_stage_buffer.terrain.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniform_stage_buffer.terrain.model =
				glm::scale(this->uniform_stage_buffer.terrain.model, glm::vec3(100.95f));
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

	class TerrainGLSample : public GLSample<Terrain> {
	  public:
		TerrainGLSample() : GLSample<Terrain>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
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
