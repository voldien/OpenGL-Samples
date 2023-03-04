#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

namespace glsample {

	class SimpleOcean : public GLSampleWindow {
	  public:
		SimpleOcean() : GLSampleWindow() {
			this->setTitle("Simple Ocean");
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

		struct UniformOceanBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 lookDirection;
			glm::vec4 direction = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
			glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			glm::vec4 ambientLight = glm::vec4(0.4, 0.4, 0.4, 1.0f);
			glm::vec4 position;

			float time = 0.0f;
			float speed = 1.0f;
			float freq = 2.0f;
			float amplitude = 1.0f;
		};

		struct UniformBufferBlock {
			UniformSkyBoxBufferBlock skybox;
			UniformOceanBufferBlock ocean;

		} uniform_stage_buffer;

		/*	*/
		GeometryObject plan;
		GeometryObject skybox;

		/*	*/
		unsigned int normal_texture;
		unsigned int reflection_texture;

		/*	*/
		unsigned int simpleOcean_program;
		unsigned int skybox_program;

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformBufferSize = sizeof(UniformBufferBlock);
		size_t skyboxUniformSize = 0;
		size_t oceanUniformSize = 0;

		class SimpleOceanSettingComponent : public nekomimi::UIComponent {

		  public:
			SimpleOceanSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Simple Ocean Settings");
			}
			virtual void draw() override {
				/*	*/
				ImGui::TextUnformatted("Light Setting");
				ImGui::ColorEdit4("Light", &this->uniform.ocean.lightColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::ColorEdit4("Ambient", &this->uniform.ocean.ambientLight[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				/*	*/
				ImGui::TextUnformatted("Ocean");
				ImGui::DragFloat("Speed", &this->uniform.ocean.speed);
				ImGui::DragFloat("Freqency", &this->uniform.ocean.freq);
				ImGui::DragFloat("Amplitude", &this->uniform.ocean.amplitude);
				/*	*/
				ImGui::TextUnformatted("Skybox");
				ImGui::ColorEdit4("Tint", &this->uniform.skybox.tintColor[0],
								  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Exposure", &this->uniform.skybox.exposure);
				/*	*/
				ImGui::TextUnformatted("Debug");

				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct UniformBufferBlock &uniform;
		};
		std::shared_ptr<SimpleOceanSettingComponent> simpleOceanSettingComponent;

		CameraController camera;

		const std::string vertexSimpleOceanShaderPath = "Shaders/simpleocean/simpleocean.vert.spv";
		const std::string fragmentSimpleOceanShaderPath = "Shaders/simpleocean/simpleocean.frag.spv";

		const std::string vertexSkyboxPanoramicShaderPath = "Shaders/skybox/skybox.vert.spv";
		const std::string fragmentSkyboxPanoramicShaderPath = "Shaders/skybox/panoramic.frag.spv";

		virtual void Release() override {
			/*	*/
			glDeleteProgram(this->simpleOcean_program);

			/*	*/
			glDeleteTextures(1, (const GLuint *)&this->reflection_texture);
			glDeleteTextures(1, (const GLuint *)&this->normal_texture);

			glDeleteBuffers(1, &this->uniform_buffer);

			/*	*/
			glDeleteVertexArrays(1, &this->plan.vao);
			glDeleteBuffers(1, &this->plan.vbo);
			glDeleteBuffers(1, &this->plan.ibo);
		}

		virtual void Initialize() override {

			std::string normalTexturePath = "asset/normalmap.png";

			const std::string panoramicPath = this->getResult()["skybox-texture"].as<std::string>();

			/*	Load shader source.	*/
			const std::vector<uint32_t> vertex_simple_ocean_binary =
				IOUtil::readFileData<uint32_t>(vertexSimpleOceanShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_simple_ocean_binary =
				IOUtil::readFileData<uint32_t>(fragmentSimpleOceanShaderPath, this->getFileSystem());

			const std::vector<uint32_t> vertex_skybox_binary =
				IOUtil::readFileData<uint32_t>(vertexSkyboxPanoramicShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_skybox_binary =
				IOUtil::readFileData<uint32_t>(fragmentSkyboxPanoramicShaderPath, this->getFileSystem());

			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*	Load shader programs.	*/
			this->simpleOcean_program = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_simple_ocean_binary,
																		 &fragment_simple_ocean_binary);
			this->skybox_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_skybox_binary, &fragment_skybox_binary);

			/*	Setup graphic pipeline settings.    */
			glUseProgram(this->simpleOcean_program);
			int uniform_buffer_index = glGetUniformBlockIndex(this->simpleOcean_program, "UniformBufferBlock");
			glUniform1i(glGetUniformLocation(this->simpleOcean_program, "ReflectionTexture"), 0);
			glUniform1i(glGetUniformLocation(this->simpleOcean_program, "NormalTexture"), 1);
			glUniformBlockBinding(this->simpleOcean_program, uniform_buffer_index, this->uniform_buffer_binding);
			glUseProgram(0);

			/*	*/
			glUseProgram(this->skybox_program);
			uniform_buffer_index = glGetUniformBlockIndex(this->skybox_program, "UniformBufferBlock");
			glUniformBlockBinding(this->skybox_program, uniform_buffer_index, 0);
			glUniform1i(glGetUniformLocation(this->skybox_program, "panorama"), 0);
			glUseProgram(0);

			/*	load Textures	*/
			TextureImporter textureImporter(this->getFileSystem());
			this->normal_texture = textureImporter.loadImage2D(normalTexturePath);
			this->reflection_texture = textureImporter.loadImage2D(panoramicPath);

			/*	Align uniform buffer in respect to driver requirement.	*/
			GLint minMapBufferSize;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &minMapBufferSize);
			this->oceanUniformSize = Math::align(sizeof(UniformOceanBufferBlock), (size_t)minMapBufferSize);
			this->skyboxUniformSize = Math::align(sizeof(UniformSkyBoxBufferBlock), (size_t)minMapBufferSize);
			this->uniformBufferSize =
				Math::align(this->skyboxUniformSize + this->oceanUniformSize, (size_t)minMapBufferSize);

			/*  Create uniform buffer.  */
			glGenBuffers(1, &this->uniform_buffer);
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			glBufferData(GL_UNIFORM_BUFFER, this->uniformBufferSize * nrUniformBuffer, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			/*	Load geometry.	*/
			std::vector<ProceduralGeometry::Vertex> vertices;
			std::vector<unsigned int> indices;
			ProceduralGeometry::generatePlan(1, vertices, indices, 256, 256);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->plan.vao);
			glBindVertexArray(this->plan.vao);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->plan.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, plan.vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProceduralGeometry::Vertex), vertices.data(),
						 GL_STATIC_DRAW);

			/*  Create index buffer.    */
			glGenBuffers(1, &this->plan.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plan.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
			this->plan.nrIndicesElements = indices.size();

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

			std::vector<ProceduralGeometry::Vertex> verticesCube;
			std::vector<unsigned int> indicesCube;
			ProceduralGeometry::generateCube(1, verticesCube, indicesCube);
			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->skybox.vao);
			glBindVertexArray(this->skybox.vao);

			/*	*/
			glGenBuffers(1, &this->skybox.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indicesCube[0]), indicesCube.data(),
						 GL_STATIC_DRAW);
			this->skybox.nrIndicesElements = indicesCube.size();

			/*	Create array buffer, for rendering static geometry.	*/
			glGenBuffers(1, &this->skybox.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, skybox.vbo);
			glBufferData(GL_ARRAY_BUFFER, verticesCube.size() * sizeof(ProceduralGeometry::Vertex), verticesCube.data(),
						 GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ProceduralGeometry::Vertex),
								  reinterpret_cast<void *>(12));

			glBindVertexArray(0);
		}

		virtual void draw() override {

			int width, height;
			this->getSize(&width, &height);

			this->uniform_stage_buffer.ocean.proj =
				glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.15f, 1000.0f);

			/*	*/
			glViewport(0, 0, width, height);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			/*	Optional - to display wireframe.	*/
			glPolygonMode(GL_FRONT_AND_BACK, this->simpleOceanSettingComponent->showWireFrame ? GL_LINE : GL_FILL);

			/*	Ocean.	*/
			{
				/*	*/
				glBindBufferRange(GL_UNIFORM_BUFFER, this->uniform_buffer_binding, this->uniform_buffer,
								  (this->getFrameCount() % this->nrUniformBuffer) * this->uniformBufferSize +
									  this->oceanUniformSize,
								  this->oceanUniformSize);

				glUseProgram(this->simpleOcean_program);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);

				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/*	*/
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	*/
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, this->normal_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->plan.vao);
				glDrawElements(GL_TRIANGLES, this->plan.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);
				glUseProgram(0);
			}

			/*	Skybox	*/
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
				glBindTexture(GL_TEXTURE_2D, this->reflection_texture);

				/*	Draw triangle.	*/
				glBindVertexArray(this->skybox.vao);
				glDrawElements(GL_TRIANGLES, this->skybox.nrIndicesElements, GL_UNSIGNED_INT, nullptr);
				glBindVertexArray(0);

				glStencilMask(GL_TRUE);

				glUseProgram(0);
			}
		}

		virtual void update() override {
			/*	Update Camera.	*/
			float elapsedTime = this->getTimer().getElapsed();
			this->camera.update(this->getTimer().deltaTime());

			/*	*/
			{
				this->uniform_stage_buffer.ocean.model = glm::mat4(1.0f);
				this->uniform_stage_buffer.ocean.model = glm::rotate(this->uniform_stage_buffer.ocean.model,
																	 glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				this->uniform_stage_buffer.ocean.model =
					glm::scale(this->uniform_stage_buffer.ocean.model, glm::vec3(10.95f));
				this->uniform_stage_buffer.ocean.view = this->camera.getViewMatrix();
				this->uniform_stage_buffer.ocean.lookDirection = glm::vec4(this->camera.getLookDirection(), 0);
				this->uniform_stage_buffer.ocean.modelViewProjection = this->uniform_stage_buffer.ocean.proj *
																	   this->uniform_stage_buffer.ocean.view *
																	   this->uniform_stage_buffer.ocean.model;
				this->uniform_stage_buffer.ocean.position = glm::vec4(this->camera.getPosition(), 0);
				this->uniform_stage_buffer.ocean.time = elapsedTime;
			}

			/*	*/
			{
				glm::quat rotation = glm::quatLookAt(glm::normalize(this->camera.getLookDirection()),
													 glm::normalize(this->camera.getUp()));
				this->uniform_stage_buffer.skybox.proj = this->uniform_stage_buffer.ocean.proj;
				this->uniform_stage_buffer.skybox.modelViewProjection =
					(this->uniform_stage_buffer.skybox.proj * glm::mat4_cast(rotation));
			}

			/*  */
			glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
			uint8_t *uniformPointer = static_cast<uint8_t *>(glMapBufferRange(
				GL_UNIFORM_BUFFER, ((this->getFrameCount() + 1) % this->nrUniformBuffer) * this->uniformBufferSize,
				this->uniformBufferSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));

			/*	*/
			memcpy(uniformPointer + 0, &this->uniform_stage_buffer.skybox, sizeof(this->uniform_stage_buffer.skybox));
			/*	*/
			memcpy(uniformPointer + this->oceanUniformSize, &this->uniform_stage_buffer.ocean,
				   sizeof(this->uniform_stage_buffer.ocean));

			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
	}; // namespace glsample

	class SimpleOceanGLSample : public GLSample<SimpleOcean> {
	  public:
		SimpleOceanGLSample() : GLSample<SimpleOcean>() {}
		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,skybox-texture", "Skybox Texture Path",
					cxxopts::value<std::string>()->default_value("asset/winter_lake_01_4k.exr"));
		}
	};

} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SimpleOceanGLSample sample;

		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}