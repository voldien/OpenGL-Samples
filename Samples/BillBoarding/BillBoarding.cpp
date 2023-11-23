#include "GLSampleSession.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class BillBoarding : public GLSampleWindow {
	  public:
		BillBoarding() : GLSampleWindow() {
			this->setTitle("BillBoarding");

			/*	Setting Window.	*/
			this->billboardSettingComponent =
				std::make_shared<BillBoardSettingComponent>(this->uniformBuffer, this->skybox);
			this->addUIComponent(this->billboardSettingComponent);

			/*	Default camera position and orientation.	*/
			this->camera.setPosition(glm::vec3(-2.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 ViewProj;
			glm::mat4 modelViewProjection;

			glm::vec4 tintColor = glm::vec4(1, 1, 1, 0.8f);

			/*	light source.	*/
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0.0f, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);

			/*	*/
			glm::vec4 cameraPosition;
			glm::vec2 scale = glm::vec2(1);
			glm::vec4 offset;

		} uniformBuffer;

		/*	*/
		GeometryObject terrain;
		GeometryObject billboard;
		Skybox skybox;

		/*	Textures.	*/
		unsigned int billboard_diffuse_texture;
		unsigned int ground_diffuse_texture;

		/*	*/
		unsigned int billboarding_program;
		unsigned int skybox_program;
		unsigned int terrain_program;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);

		CameraController camera;

		class BillBoardSettingComponent : public nekomimi::UIComponent {
		  public:
			BillBoardSettingComponent(struct UniformBufferBlock &uniform, Skybox &skybox)
				: uniform(uniform), skybox(skybox) {
				this->setName("BillBoarding Setting");
			}

			void draw() override {
				ImGui::ColorEdit4("Tint", &this->uniform.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat3("Direction", &this->uniform.direction[0]);
				ImGui::ColorEdit4("Ambient", &this->uniform.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

				ImGui::TextUnformatted("BillBoarding Setting");
				ImGui::DragFloat2("Scale", &this->uniform.scale[0]);
				ImGui::TextUnformatted("Debug Setting");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
				ImGui::Checkbox("Show Billboard", &this->showBillBoards);
				skybox.RenderImGUI();
			}

			bool showWireFrame = false;
			bool showBillBoards = true;

		  private:
			struct UniformBufferBlock &uniform;
			Skybox &skybox;
		};
		std::shared_ptr<BillBoardSettingComponent> billboardSettingComponent;

		/*	BillBoard Shader Paths.	*/
		const std::string vertexBillBoardShaderPath = "Shaders/billboarding/billboarding.vert.spv";
		const std::string geomtryBillBoardShaderPath = "Shaders/billboarding/billboarding.geom.spv";
		const std::string fragmentBillBoardShaderPath = "Shaders/billboarding/billboarding.frag.spv";

		/*	Simple Terrain.	*/
		const std::string vertexTerrainShaderPath = "Shaders/terrain/terrain.vert";
		const std::string fragmentTerrainShaderPath = "Shaders/terrain/terrain.frag";

		/*	Skybox.	*/
		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		void Release() override {
			/*	Delete graphic pipeline.	*/
			glDeleteProgram(this->billboarding_program);
			glDeleteProgram(this->skybox_program);
			glDeleteProgram(this->terrain_program);

			/*	Delete texture.	*/
			glDeleteTextures(1, (const GLuint *)&this->billboard_diffuse_texture);
			glDeleteTextures(1, (const GLuint *)&this->ground_diffuse_texture);

			/*	Delete uniform buffer.	*/
			glDeleteBuffers(1, &this->uniform_buffer);

			/*	Delete geometry data.	*/
			glDeleteVertexArrays(1, &this->terrain.vao);
			glDeleteBuffers(1, &this->terrain.vbo);
			glDeleteBuffers(1, &this->terrain.ibo);

			glDeleteVertexArrays(1, &this->billboard.vao);
			glDeleteBuffers(1, &this->billboard.vbo);
		}

		void Initialize() override {

			const std::string diffuseBillboardTexturePath = this->getResult()["texture"].as<std::string>();
			const std::string diffuseGroundTexturePath = this->getResult()["ground-texture"].as<std::string>();
			const std::string skyboxPath = this->getResult()["skybox"].as<std::string>();

			{
				/*	Load shader source.	*/
				const std::vector<uint32_t> vertex_source =
					IOUtil::readFileData<uint32_t>(this->vertexBillBoardShaderPath, this->getFileSystem());
				const std::vector<uint32_t> geomtry_source =
					IOUtil::readFileData<uint32_t>(this->geomtryBillBoardShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_source =
					IOUtil::readFileData<uint32_t>(this->fragmentBillBoardShaderPath, this->getFileSystem());

				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Load shader	*/
				this->billboarding_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source,
																			  &fragment_source, &geomtry_source);

				/*	Load shader	*/
				const std::vector<char> simple_terrain_vertex_source =
					IOUtil::readFileString(vertexTerrainShaderPath, this->getFileSystem());
				const std::vector<char> simple_terrain_fragment_source =
					IOUtil::readFileString(fragmentTerrainShaderPath, this->getFileSystem());

				this->terrain_program =
					ShaderLoader::loadGraphicProgram(&simple_terrain_vertex_source, &simple_terrain_fragment_source);

				/*	Load shader binaries.	*/
				const std::vector<uint32_t> vertex_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->vertexSkyboxPanoramicShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_skybox_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

				/*	*/
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*	Create skybox graphic pipeline program.	*/
				this->skybox_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);
			}

			/*	Setup Billboard graphic pipeline.	*/
			glUseProgram(this->billboarding_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->billboarding_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->billboarding_program, "DiffuseTexture"), 0);
			glUniformBlockBinding(this->billboarding_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	Setup Terrain graphic pipeline.	*/
			glUseProgram(this->terrain_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->terrain_program, "UniformBufferBlock");
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
			this->billboard_diffuse_texture =
				textureImporter.loadImage2D(diffuseBillboardTexturePath, ColorSpace::SRGB);
			this->ground_diffuse_texture = textureImporter.loadImage2D(diffuseGroundTexturePath, ColorSpace::SRGB);
			unsigned int skytexture = textureImporter.loadImage2D(skyboxPath, ColorSpace::SRGB);
			skybox.Init(skytexture, this->skybox_program);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->uniformBufferSize = Math::align(this->uniformBufferSize, (size_t)minMapBufferSize);

			/*	Create uniform buffer.	*/
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * this->nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			{
				/*	Load geometry.	*/
				std::vector<ProceduralGeometry::Vertex> vertices;
				std::vector<unsigned int> indices;
				ProceduralGeometry::generatePlan(10, vertices, indices, 512, 512);

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

					vertices[i].vertex[0] *= 20.0f;
					vertices[i].vertex[1] *= 20.0f;
					vertices[i].vertex[2] = height;
				}

				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->terrain.vao);
				glBindVertexArray(this->terrain.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glGenBuffers(1, &this->terrain.vbo);
				glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex),
							 vertices.data(), GL_STATIC_DRAW);

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

				/*	Billboarding.	*/
				/*	Create array buffer, for rendering static geometry.	*/
				glGenVertexArrays(1, &this->billboard.vao);
				glBindVertexArray(this->billboard.vao);

				/*	Create array buffer, for rendering static geometry.	*/
				glBindBuffer(GL_ARRAY_BUFFER, terrain.vbo);

				std::vector<unsigned int> billboardIndices;
				billboardIndices.resize(1024);
				for (size_t i = 0; i < billboardIndices.size(); i++) {
					billboardIndices[i] = std::abs(std::rand()) % vertices.size();
				}
				/*	*/
				glGenBuffers(1, &this->billboard.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboard.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, billboardIndices.size() * sizeof(indices[0]),
							 billboardIndices.data(), GL_STATIC_DRAW);
				this->billboard.nrIndicesElements = billboardIndices.size();
				/*	Vertex.	*/
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

				glBindVertexArray(0);
			}
		}

		void onResize(int width, int height) override {
			/*	*/
			this->camera.setAspect((float)width / (float)height);
		}

		void draw() override {

			int width, height;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.095f, 0.095f, 0.095f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// TODO fix terrain.
			/*	Draw terrain.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->terrain_program);

				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				glFrontFace(GL_CCW);

				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	Blending.	*/
				glDisable(GL_BLEND);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->ground_diffuse_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, this->billboardSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw triangle.	*/
				glBindVertexArray(this->terrain.vao);
				glDrawElements(GL_TRIANGLES, this->terrain.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			if (this->billboardSettingComponent->showBillBoards) {
				/*	Draw billboard.	*/

				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize,
								  this->uniformBufferSize);

				glUseProgram(this->billboarding_program);

				glDisable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				glFrontFace(GL_CCW);

				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);

				/*	Blending.	*/
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->billboard_diffuse_texture);

				/*	Optional - to display wireframe.	*/
				glPolygonMode(GL_FRONT_AND_BACK, billboardSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

				/*	Draw triangle.	*/
				glBindVertexArray(this->billboard.vao);
				glDrawElements(GL_POINTS, this->billboard.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
			}

			/*	*/
			this->skybox.Render(this->camera);
		}

		void update() override {

			/*	Update Camera.	*/
			const float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			this->uniformBuffer.proj = this->camera.getProjectionMatrix();
			this->uniformBuffer.model = glm::mat4(1.0f);
			this->uniformBuffer.model =
				glm::rotate(this->uniformBuffer.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			this->uniformBuffer.model = glm::scale(this->uniformBuffer.model, glm::vec3(10.95f));
			this->uniformBuffer.view = this->camera.getViewMatrix();
			this->uniformBuffer.modelViewProjection =
				this->uniformBuffer.proj * this->uniformBuffer.view * this->uniformBuffer.model;
			this->uniformBuffer.ViewProj = this->uniformBuffer.proj * this->uniformBuffer.view;
			this->uniformBuffer.cameraPosition = glm::vec4(this->camera.getPosition(), 0);

			/*	*/
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			void *uniformPointer = glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
			memcpy(uniformPointer, &this->uniformBuffer, sizeof(this->uniformBuffer));
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	};

	class BillBoardingGLSample : public GLSample<BillBoarding> {
	  public:
		BillBoardingGLSample() : GLSample<BillBoarding>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("G,ground-texture", "Ground Texture",
					cxxopts::value<std::string>()->default_value("asset/stylized-ground.png"))(
				"T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/stylized-tree.png"))(
				"S,skybox", "Skybox Texture Path (Panoramic)",
				cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::BillBoardingGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}